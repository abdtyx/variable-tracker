#include "pin.H"
#include "typehelper.gen.hh"
#include <iostream>

using std::cout, std::endl, std::set;
using vt::lock_acquire, vt::lock_release, vt::var_set, vt::var;

VOID RecordRead(VOID *addr) {
    lock_acquire();
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_read((*it));
    }
    lock_release();
}

// When the address of global variable is overwrited, this function is called to calculate the new address for all its fields
VOID BeforeWrite(VOID *addr) {
    lock_acquire();
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_before_write((*it));
        (*it)->set_before_write(*it);
    }
    lock_release();
}

// When the address of global variable is overwrited, this function is called to calculate the new address for all its fields
VOID AfterWrite(VOID *addr) {
    lock_acquire();
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_after_write((*it));
        (*it)->set_after_write(*it, DEFAULT_DELIMITER);
        // print_var(*it);
    }
    lock_release();
}

VOID BeforeFree(VOID* addr) {
    lock_acquire();
    if (addr != NULL)
        cout << "[CALL] [free(" << addr << ")]" << endl;
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        // find all children
        set<var*> children;
        for (auto i : (*it)->father)
            for (auto j : i->children)
                children.insert(j);
        // find all fathers
        set<var*> fathers;
        for (auto i : children)
            for (auto j : i->father)
                fathers.insert(j);
        // call set_before_write on all fathers
        for (auto i : fathers)
            i->set_before_write(i);
    }
    lock_release();
}

VOID BeforeLeave(ADDRINT rbp, ADDRINT rsp) {
    lock_acquire();
    // cout << var_set.size() << endl;
    uint64_t start = rsp, end = rbp + 0x10;
    vector<var*> dirty;
    for (auto i : var_set) {
        if ((uint64_t)i->address >= start && (uint64_t)i->address < end) {
            dirty.push_back(i);     
        }
    }
    // find all fathers
    set<var*> fathers;
    for (auto i : dirty)
        for (auto j : i->father)
            fathers.insert(j);
    // call set_before_write on all fathers
    for (auto i : fathers)
        i->set_before_write(i);
    // clean the rest of the dirty memory
    for (auto i : dirty) {
        if (var_set.erase(i))
            free(i);
    }
    // ADDRINT rbp = PIN_GetContextReg(c, REG_GBP);
    // ADDRINT rsp = PIN_GetContextReg(c, REG_STACK_PTR);
    // printf("rbp value: %lx\n", rbp);
    // printf("rsp value: %lx\n", rsp);
    // cout << "rbp value: " << rbp << endl;
    // cout << "rsp value: " << rsp << endl;
    // cout << var_set.size() << endl;
    lock_release();
}

VOID Image(IMG img, VOID* v) {
    // search application name
    string app_name = IMG_Name(img);
    if (app_name.find("/home") == 0) {
        cout << "Target program: " << app_name << endl;
        // Initialize cvs
        vt::cvs_init(app_name);
    }
    // Find the free() function.
    RTN freeRtn = RTN_FindByName(img, "free");
    if (RTN_Valid(freeRtn)) {
        RTN_Open(freeRtn);

        RTN_InsertCall(
            freeRtn,
            IPOINT_BEFORE,
            (AFUNPTR)BeforeFree,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END
        );

        RTN_Close(freeRtn);
    }
}

VOID InsertInstruction(INS ins, VOID *v) {
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordRead,
                IARG_MEMORYOP_EA, memOp,
                IARG_END
            );
        }
        if (INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)BeforeWrite,
                IARG_MEMORYOP_EA, memOp,
                IARG_END
            );
        }
        if (INS_MemoryOperandIsWritten(ins, memOp) && INS_IsValidForIpointAfter(ins)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_AFTER, (AFUNPTR)AfterWrite,
                IARG_MEMORYOP_EA, memOp,
                IARG_END
            );
        }
    }
    // cout << INS_Disassemble(ins) << endl;
    if (INS_Disassemble(ins) == "leave ") {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)BeforeLeave,
            IARG_REG_VALUE, REG_GBP,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_END
        );
    }
}

int main(int argc, char *argv[]) {
    // Initialize pin
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return -1;

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(InsertInstruction, 0);

    IMG_AddInstrumentFunction(Image, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

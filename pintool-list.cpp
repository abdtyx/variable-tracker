#include "pin.H"
#include "typehelper.gen.hh"
#include <iostream>

using std::cout, std::endl, std::set;

static VOID* l_address = (VOID*)0x555555558158;
static VOID* l2_address = (VOID*)0x555555558160;

VOID RecordRead(VOID *addr) {
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_read((*it));
    }
}

// When the address of global variable is overwrited, this function is called to calculate the new address for all its fields
VOID BeforeWrite(VOID *addr) {
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_before_write((*it));
        if ((*it)->type == TYPE_POINTER) {
            (*it)->set_before_write(*it);
        }
    }
}

// When the address of global variable is overwrited, this function is called to calculate the new address for all its fields
VOID AfterWrite(VOID *addr) {
    var to_search;
    to_search.address = addr;
    auto it = var_set.find(&to_search);
    if (it != var_set.end()) {
        (*it)->log_after_write((*it));
        if ((*it)->type == TYPE_POINTER) {
            (*it)->set_after_write(*it);
        }
    }
}

VOID BeforeFree(VOID* addr) {
    cout << "[CALL] [free(" << addr << ")]" << endl;
    if (ptr_addr.find(addr) != ptr_addr.end()) {
        set<void*> value = ptr_addr[addr];
        ptr_addr.erase(addr);
        for (auto i = value.begin(); i != value.end(); i++) {
            var to_search;
            to_search.address = *i;
            auto it = var_set.find(&to_search);
            if (i == value.begin()) {
                if (it != var_set.end()) {
                    if ((*it)->type == TYPE_POINTER) {
                        (*it)->set_before_write(*it);
                    }
                }
            } else {
                if (it != var_set.end()) {
                    if ((*it)->type == TYPE_POINTER) {
                        // void* value;
                        // PIN_SafeCopy(&value, (*it)->address, sizeof(value));
                        // ptr_addr_remove(value, (*it)->address);
                        (*it)->children.clear();
                    }
                }
            }
        }
    }
    // var to_search;
    // to_search.address = addr;
    // auto it = var_set.find(&to_search);
    // if (it != var_set.end()) {
    //     print_var(*it);
    //     if ((*it)->type == TYPE_POINTER) {
    //         (*it)->set_before_write(*it);
    //         var_set.erase(it);
    //         // copy addr?
    //         free(*it);
    //     }
    // }
}

VOID Image(IMG img, VOID* v) {
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
}

int main(int argc, char *argv[]) {
    // Initialize var set
    var* list_var_l = list_var_construct((void*)l_address, TYPE_POINTER, "l");
    var_set.insert(list_var_l);

    var* list_var_l2 = list_var_construct((void*)l2_address, TYPE_POINTER, "l2");
    var_set.insert(list_var_l2);

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

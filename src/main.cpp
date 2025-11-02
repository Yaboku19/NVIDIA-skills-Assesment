#include "wasm_interpreter.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: wasm_interpreter <file.wat>\n";
        return 1;
    }

    std::string filename = argv[1];
    try {
        WasmInterpreter interpreter;
        interpreter.loadFile(filename);
        interpreter.parse();
        interpreter.callFunctionByExportName("_start");
        interpreter.callFunctionByExportName("_test_addition");
        interpreter.callFunctionByExportName("_test_subtraction");
        interpreter.callFunctionByExportName("_test_multiplication");
        interpreter.callFunctionByExportName("_test_division_signed");
        interpreter.callFunctionByExportName("_test_division_unsigned");
        interpreter.callFunctionByExportName("_test_remainder");
        interpreter.callFunctionByExportName("_test_and");
        interpreter.callFunctionByExportName("_test_or");
        interpreter.callFunctionByExportName("_test_xor");
        interpreter.callFunctionByExportName("_test_shift_left");
        interpreter.callFunctionByExportName("_test_shift_right_signed");
        interpreter.callFunctionByExportName("_test_shift_right_unsigned");
        interpreter.callFunctionByExportName("_test_store_load");
        interpreter.callFunctionByExportName("_test_store_load_byte_unsigned");
        interpreter.callFunctionByExportName("_test_store_load_byte_signed");
        interpreter.callFunctionByExportName("_test_locals_arithmetic");
        interpreter.callFunctionByExportName("_test_locals_tee");
        //interpreter.callFunctionByExportName("_test_global_increment");
        //interpreter.showMemory(0, 128);
        //interpreter.callFunctionByExportName("_test_global_constant");
        //interpreter.showMemory(0, 128);
        //interpreter.callFunctionByExportName("_test_global_multiple");
        //interpreter.showMemory(0, 128);
        interpreter.callFunctionByExportName("_test_combined");
        interpreter.callFunctionByExportName("_test_eq");
        interpreter.callFunctionByExportName("_test_ne");
        interpreter.callFunctionByExportName("_test_lt_s");
        interpreter.callFunctionByExportName("_test_lt_u");
        interpreter.callFunctionByExportName("_test_gt_s");
        interpreter.callFunctionByExportName("_test_gt_u");
        interpreter.callFunctionByExportName("_test_le_s");
        interpreter.callFunctionByExportName("_test_ge_s");
        interpreter.callFunctionByExportName("_test_eqz_zero");
        interpreter.callFunctionByExportName("_test_eqz_nonzero");
        interpreter.callFunctionByExportName("_test_clz");
        interpreter.callFunctionByExportName("_test_ctz");
        interpreter.callFunctionByExportName("_test_popcnt");
        interpreter.callFunctionByExportName("_test_popcnt_all");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

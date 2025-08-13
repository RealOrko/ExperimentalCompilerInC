#include "compiler.h"
#include "debug.h"
#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    CompilerOptions options;
    Module *module = NULL;

    compiler_init(&options, argc, argv);
    init_debug(options.log_level);
    module = compiler_compile(&options);

    if (module == NULL) {
        compiler_cleanup(&options);
        return 1;
    }

    CodeGen gen;
    code_gen_init(&options.arena, &gen, &options.symbol_table, options.output_file);
    code_gen_module(&gen, module);
    code_gen_cleanup(&gen);

    compiler_cleanup(&options);

    return 0;
}
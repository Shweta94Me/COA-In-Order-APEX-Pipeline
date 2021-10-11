/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    case OPCODE_LDR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }

    case OPCODE_LOAD:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    case OPCODE_NOP:
    {
        printf("%s", stage->opcode_str);
        break;
    }

    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1, stage->imm);
        break;
    }

    case OPCODE_STR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2, stage->rs3);
        break;
    }

    case OPCODE_CMP:
    {
        printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

/*Shweta : To handle errors*/
int errorHandler(char *errorType)
{
    if (strcmp(errorType, "MemoryError") == 0)
    {
        printf("Error ::: Requested effective memory address if out of range (0-4096).\n");
        exit(1);
    }
    return 0;
}

/*Shweta : Data Forwarding - Check if source registers match with destination register of intsruction in EX stage*/
void dataForwarding(APEX_CPU *cpu, int hasrs1, int hasrs2, int hasrs3)
{
    /*Shweta : Flag variables to check source registers are FREE (0) or BUSY (1)*/
    int rs1_valid = 0;
    int rs2_valid = 0;
    int rs3_valid = 0;
    
    cpu->decode.stalled = 0; //By Default keep the decode stage as unstalled

    if (hasrs1 && hasrs2 && hasrs3) //Instruction with three source registers but one or more may be INVALID eg. STR
    {
        if(cpu->registerStatus[cpu->decode.rs1]){
            rs1_valid = 1; //BUSY
        }

        if(cpu->registerStatus[cpu->decode.rs2]){
            rs2_valid = 1; //BUSY
        }

        if(cpu->registerStatus[cpu->decode.rs3]){
            rs3_valid = 1; //BUSY
        }

        /*Check if there is any instruction already in pipeline with
        same destionation register matches with source register*/

        if(rs1_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs1)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs1_value = cpu->forwardRegister[cpu->decode.rs1];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
        }

        if(rs2_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs2)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs2_value = cpu->forwardRegister[cpu->decode.rs2];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
        }

        if(rs3_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs3)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs3_value = cpu->forwardRegister[cpu->decode.rs3];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs3_value = cpu->regs[cpu->decode.rs3];
        }
    }
    else if (hasrs1 && hasrs2 && !hasrs3) //Instruction with two source registers but one or more may be INVALID eg. ADD, SUB, MUL, AND, OR, EX-OR, LDR, CMP, STORE
    {
        if(cpu->registerStatus[cpu->decode.rs1]){
            rs1_valid = 1; //BUSY
        }

        if(cpu->registerStatus[cpu->decode.rs2]){
            rs2_valid = 1; //BUSY
        }

        /*Check if there is any instruction already in pipeline with
        same destionation register matches with source register*/

        if(rs1_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs1)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs1_value = cpu->forwardRegister[cpu->decode.rs1];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
        }

        if(rs2_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs2)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs2_value = cpu->forwardRegister[cpu->decode.rs2];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
        }

        /*Shweta : do not set destination as BUSY (1) for STORE & CMP instruction as there is no destionation in it*/
        if(!cpu->decode.stalled && strcmp(cpu->decode.opcode_str, "STORE") != 0 && strcmp(cpu->decode.opcode_str, "CMP") != 0){
            cpu->registerStatus[cpu->decode.rd] = 1;
        }
    }
    else if (hasrs1 && !hasrs2 && !hasrs3) //Instruction with two source registers but one or more may be INVALID eg. LOAD, ADDL, SUBL
    {
        if(cpu->registerStatus[cpu->decode.rs1]){
            rs1_valid = 1; //BUSY
        }

        /*Check if there is any instruction already in pipeline with
        same destionation register matches with source register*/

        if(rs1_valid){

            //Look for forwarding only if next stage is not LOAD as LOAD produces result in MEM stage
            if((strcmp(cpu->execute.opcode_str, "LOAD") == 0 || strcmp(cpu->execute.opcode_str, "LDR") == 0)
            && cpu->execute.rd == cpu->decode.rs1)
            {
                cpu->decode.stalled = 1;
            }
            else
            {
                //Get the latest value from forwardRegister
                cpu->decode.rs1_value = cpu->forwardRegister[cpu->decode.rs1];
            }
        }
        else{
            //Data forwarding is not possible; now look into register file 
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
        }

        if(!cpu->decode.stalled){
            cpu->registerStatus[cpu->decode.rd] = 1;
        }
    }
}

void createNOPInstr(APEX_CPU *cpu)
{
    cpu->execute.opcode = 0x13; //Pass NOP opcode 
    strcpy(cpu->execute.opcode_str, "NOP"); //Pass opcode_str as NOP
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (!cpu->fetch.stalled) /*if fetch stage is not stalled */
    {
        if (cpu->fetch.has_insn)
        {
            /* This fetches new branch target instruction from next cycle */
            if (cpu->fetch_from_next_cycle == TRUE)
            {
                cpu->fetch_from_next_cycle = FALSE;

                /* Skip this cycle*/
                return;
            }

            /* Store current PC in fetch latch */
            cpu->fetch.pc = cpu->pc;

            /* Index into code memory using this pc and copy all instruction fields
            * into fetch latch  */
            current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
            strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
            cpu->fetch.opcode = current_ins->opcode;
            cpu->fetch.rd = current_ins->rd;
            cpu->fetch.rs1 = current_ins->rs1;
            cpu->fetch.rs2 = current_ins->rs2;
            cpu->fetch.rs3 = current_ins->rs3;
            cpu->fetch.imm = current_ins->imm;

            if (!cpu->decode.stalled)
            {
                /* Update PC for next instruction */
                cpu->pc += 4;

                /* Copy data from fetch latch to decode latch*/
                cpu->decode = cpu->fetch;
            }
            else
            {
                cpu->fetch.stalled = 1;
            }

            if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
            {
                print_stage_content("Fetch", &cpu->fetch);
            }
            /* Stop fetching new instructions if HALT is fetched */
            if (cpu->fetch.opcode == OPCODE_HALT && !cpu->decode.stalled)
            {
                cpu->fetch.has_insn = FALSE;
            }
        }
    }
    else if (cpu->fetch.stalled) /*Fetch stage is stalled*/
    {
        if (!cpu->decode.stalled)
        {
            cpu->fetch.stalled = 0;
            /* Update PC for next instruction */
            cpu->pc += 4;

            /* Copy data from fetch latch to decode latch*/
            cpu->decode = cpu->fetch;
        }
        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT && !cpu->decode.stalled)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_LDR:
            case OPCODE_CMP:
            case OPCODE_STORE:
            {
                
                dataForwarding(cpu, 1, 1, 0);
                break;
            }

            case OPCODE_LOAD:
            {
                dataForwarding(cpu, 1, 0, 0);
                break;
            }

            case OPCODE_MOVC:
            {
                /*Shweta : MOVC instruction doesn't have any source register*/
                if (!cpu->registerStatus[cpu->decode.rd])
                {
                    cpu->registerStatus[cpu->decode.rd] = 1;
                    cpu->decode.stalled = 0;
                }
                else
                {
                    cpu->decode.stalled = 1;
                }
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                dataForwarding(cpu, 1, 0, 0);
                break;
            }

            case OPCODE_STR:
            {
                dataForwarding(cpu, 1, 1, 1);
                break;
            }
            case OPCODE_HALT:
            {
                cpu->decode.stalled = 0;
                break;
            }
        }

        if (!cpu->decode.stalled)
        {
            /* Copy data from decode latch to execute latch*/
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
        }
        else{
            /*Shweta : If decode stage is stalled then pass NOP 
            instruction to excute stage to create bubble*/
            cpu->execute = cpu->decode;
            createNOPInstr(cpu);
        }
        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                if (cpu->execute.memory_address > 4096 || cpu->execute.memory_address < 0)
                {
                    errorHandler("MemoryError");
                }
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages, 
                        * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages, 
                        * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_MOVC:
            {
                cpu->execute.result_buffer = cpu->execute.imm;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_SUB:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                if (cpu->execute.memory_address > 4096 || cpu->execute.memory_address < 0)
                {
                    errorHandler("MemoryError");
                }
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
                /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
                cpu->forwardRegister[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_LDR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
                if (cpu->execute.memory_address > 4096 || cpu->execute.memory_address < 0)
                {
                    errorHandler("MemoryError");
                }
                break;
            }

            case OPCODE_STR:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.rs3_value;
                if (cpu->execute.memory_address > 4096 || cpu->execute.memory_address < 0)
                {
                    errorHandler("MemoryError");
                }
                break;
            }

            case OPCODE_CMP:
            {
                /* Set the zero flag based on the result buffer */
                if (cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_MUL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            /* No work for ADD */
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            /*Shweta : Store data in forward array to perform Data Forwarding for any future dependent instruction*/
            cpu->forwardRegister[cpu->memory.rd] = cpu->memory.result_buffer;
            break;
        }

        case OPCODE_STORE:
        case OPCODE_STR:
        {
            /*Store data from destination register to data memory */
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
            break;
        }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->registerStatus[cpu->writeback.rd] = 0;
                break;
            }

            case OPCODE_LOAD:
            case OPCODE_LDR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->registerStatus[cpu->writeback.rd] = 0;
                break;
            }

            case OPCODE_MOVC:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->registerStatus[cpu->writeback.rd] = 0;
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename, const char *operation, const int cycles)
{
    int i;
    APEX_CPU *cpu;

    if (!filename && !operation && !cycles) /*Shweta : check if valid function and cylces num added by users*/
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    if(strcmp(operation, "showmem") != 0){
        cpu->operationCycles = cycles;
    }
    else{
        if (cycles > 4096 || cycles < 0)
        {
            errorHandler("MemoryError");
        }
        cpu->memLoc = cycles;
        cpu->showMem = 1;
    }
    
    /*Shweta : If asked for simulation then set simulate property or else do display*/
    if (strcmp(operation, "simulate") == 0)
    {
        cpu->simulate = 1;
    }
    else
    {
        cpu->simulate = 0;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);

    cpu->single_step = 0;
    if (strcmp(operation, "single_step") == 0)
    {
        cpu->single_step = ENABLE_SINGLE_STEP;
    }

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }


    if (ENABLE_DEBUG_MESSAGES && !cpu->simulate)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2", "rs3", 
            "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].rs3, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE) 
    {
        if (ENABLE_DEBUG_MESSAGES && !cpu->simulate) /*Shweta : Show if not simulate*/
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu) || (cpu->clock == cpu->operationCycles && !cpu->showMem)) /*Shweta : Stop execution if enconter HALT instruction 
        or excuted asked number of instructions cycles(for simulate or display)*/
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        // print_reg_file(cpu); /*Shweta : Commented this line to display content of register file at each stage for single_step functionality*/

        if (cpu->single_step)
        {
            print_reg_file(cpu); /*Shweta : added it here*/
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }

    /*Shweta : Show state of register file and data memory*/
    if (!cpu->single_step)
    {
        if(cpu->showMem){
            printf("===============STATE OF REQUESTED DATA MEMORY===============\n");
            printf("|\t\tMEM[%d]\t\t|\t\tData Value = %d\t\t|\n", cpu->memLoc, cpu->data_memory[cpu->memLoc]);
        }
        else{
            printf("===============STATE OF ARCHITECTURAL REGISTER FILE===============\n");
            for (int i = 0; i < 16; i++)
            {
                char *isValid = (cpu->registerStatus[i])? "INVALID" : "VALID"; /*Check if register is free(0) or BUSY (1)*/
                printf("|\t\tREG[%d]\t\t|\t\tVALUE=%d\t\t|\t\tSTATUS=%s\t\t|\n", i, cpu->regs[i], isValid);
            }

            printf("===============STATE OF DATA MEMORY===============\n");
            for (int i = 0; i < 10; i++)
            {
                printf("|\t\tMEM[%d]\t\t|\t\tData Value = %d\t\t|\n", i, cpu->data_memory[i]);
            }
        }
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
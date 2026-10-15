#include "cache.h"
#include <iostream>

using namespace std;

#define maxRRPV 3

#ifndef THRESHOLD
#define THRESHOLD 0.25
#endif

#define L2C_MAX_SETS 8192
#define L2C_MAX_WAYS 64

static uint32_t l2_rrpv[L2C_MAX_SETS][L2C_MAX_WAYS];

static uint64_t total_accesses = 0;
static uint64_t total_misses = 0;
static uint64_t instruction_accesses = 0;
static uint64_t instruction_misses = 0;

void CACHE::l2c_initialize_replacement() {
    for (uint32_t i = 0; i < L2C_MAX_SETS; i++) {
        for (uint32_t j = 0; j < L2C_MAX_WAYS; j++) {
            l2_rrpv[i][j] = maxRRPV;
        }
    }
    total_accesses = 0;
    total_misses = 0;
    instruction_accesses = 0;
    instruction_misses = 0;
}

uint32_t CACHE::l2c_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    uint32_t s = set % L2C_MAX_SETS;
    uint32_t ways = (NUM_WAY < L2C_MAX_WAYS) ? NUM_WAY : L2C_MAX_WAYS;


    for (uint32_t i = 0; i < ways; i++) {
        if (current_set[i].valid == 0) {
            return i;
        }
    }

    
    while (true) {
        for (uint32_t i = 0; i < ways; i++) {
            if (l2_rrpv[s][i] >= maxRRPV) {
                return i;
            }
        }

        for (uint32_t i = 0; i < ways; i++) {
            if (l2_rrpv[s][i] < maxRRPV) {
                l2_rrpv[s][i]++;
            }
        }
    }

    return 0;
}

void CACHE::l2c_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit) {
    uint32_t s = set % L2C_MAX_SETS;
    uint32_t w = way % L2C_MAX_WAYS;

    bool isInstruction = (type == LOAD || type == PREFETCH) && (ip != 0);

    total_accesses++;
    if (!hit) total_misses++;

    if (isInstruction) {
        instruction_accesses++;
        if (!hit) instruction_misses++;
    }

    double total_denom = (double)(total_accesses);
    double instruction_num = (double)(instruction_accesses);
    double instruction_ratio = (total_denom > 0) ? (instruction_num / total_denom) : 0.0;
    bool use_irrip = (instruction_ratio > THRESHOLD);

    if (hit) {
        l2_rrpv[s][w] = 0;
    } else {
        if (use_irrip && isInstruction) {
            l2_rrpv[s][w] = maxRRPV - 2; 
        } else {
            l2_rrpv[s][w] = maxRRPV - 1; 
        }
    }
}

void CACHE::l2c_replacement_final_stats() {
    double total_denom = (double)(total_accesses);
    double instruction_num = (double)(instruction_accesses);
    double ratio = (total_denom > 0) ? (instruction_num / total_denom) : 0.0;

    cout << "<<<--- Dynamic I-RRIP Stats --->>>" << endl;
    cout << "Total Accesses: " << total_accesses << " | Total Misses: " << total_misses << endl;
    cout << "Instruction Ratio: " << ratio << endl;
}
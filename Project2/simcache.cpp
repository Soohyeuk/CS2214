/*
CS-UY 2214
Adapted from Jeff Epstein
Starter code for E20 cache Simulator
simcache.cpp
*/

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <regex>
#include <iomanip>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;



/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
};

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/

//sign extend function for 7-bit imm to 13-bit imm
void signExtend(uint16_t& number) {
    if ( (number >> 6) == 1) {
        number = number | 65408;
    }
};

//Block struct that contains 16-bit tag
struct Block {
    uint16_t tag = -1;
};

//Row struct that contains associativity and vector of Block
struct Row {
    int associativity;
    vector<Block> blocks;
};

//Level struct that contains vector of Row and
//calculates how many blocks a row will contain and how many rows a level will contain
struct Level {
    int level_size = 0;
    int blocksize;
    vector<Row> rows;
    Level(int size, int assoc, int blocksize) : blocksize(blocksize) {
        level_size = size / (assoc * blocksize);
        for (size_t i = 0; i < level_size; i++) {
            Row row;
            row.associativity = assoc;
            row.blocks = vector<Block>(row.associativity);
            rows.push_back(row);
        }
    };
};

//Cache struct that contains max 2 level in a vector
struct Cache {
    vector<Level> levels;
};

void print_log_entry(const string &cache_name, const string &status, uint16_t pc, uint16_t addr, int row) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
        " pc:" << setw(5) << pc <<
        "\taddr:" << setw(5) << addr <<
        "\trow:" << setw(4) << row << endl;
}
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
        ", associativity " << assoc << ", blocksize " << blocksize <<
        ", rows " << num_rows << endl;
}

//just a helper function that deals with LRU and miss/hit
bool cache_helper(Row* row, uint16_t tagID) {
    bool miss_check = true;
    for (size_t i = 0; i < row->blocks.size(); i++) {                //iteration of blocks in the row
            if (row->blocks[i].tag == tagID) {                           // hit
                row->blocks.erase(row->blocks.begin() + i);
                Block temp;
                temp.tag = tagID;
                row->blocks.push_back(temp);
                miss_check = false;
            }
        }
        if (miss_check == true) {                                   // if it is a fully miss
            row->blocks.erase(row->blocks.begin());

            Block temp;
            temp.tag = tagID;
            row->blocks.push_back(temp);
        }
    return miss_check;
}

//cache call function that calculates row, tag, if it should check L2, and what it should print
void cache_func(uint16_t memory, const uint16_t pc, Cache& cache, bool check) {
    //L1 cache information
    uint16_t blockID = memory / cache.levels[0].blocksize;
    uint16_t row = blockID % cache.levels[0].level_size;
    uint16_t tagID = blockID / cache.levels[0].level_size;
    
    //for ease of read
    Level* L1level = &(cache.levels[0]);
    Row* L1rows = &(L1level->rows[row]);

    //calls a helper function to calculate then checks what to print out
    bool is_miss = cache_helper(L1rows, tagID);
    if (check == false) {
        print_log_entry("L1", "SW", pc, memory, row);
    } else if (is_miss == true) {
        print_log_entry("L1", "MISS", pc, memory, row);
    } else {
         print_log_entry("L1", "HIT", pc, memory, row);
    }
    
    
    if (cache.levels.size() == 2 && (is_miss || !check)) {  //sw goes to both regardless, but lw has to check if it's a miss first
        //L2 cache information
        blockID = memory / cache.levels[1].blocksize;
        row = blockID % cache.levels[1].level_size;
        tagID = blockID / cache.levels[1].level_size;
        
        //for ease of read
        Level* L2level = &(cache.levels[1]);
        Row* L2rows = &(L2level->rows[row]);

        //calls a helper function to calculate then checks what to print out
        is_miss = cache_helper(L2rows, tagID);
        if (check == false) {
        print_log_entry("L2", "SW", pc, memory, row);
        } else if (is_miss == true) {
            print_log_entry("L2", "MISS", pc, memory, row);
        } else {
            print_log_entry("L2", "HIT", pc, memory, row);
        }
    }
};

//E20 simulator
void simulator(uint16_t& pc, uint16_t R[], uint16_t memory[], Cache& cache) {
    // TODO: your code here. Do simulation.
    bool halt = false;
    while (!halt) {
        uint16_t position = pc;
        uint16_t line = memory[position % 8192];
        //finds all the binary code we need for any instructions.
        uint16_t opCode = line >> 13;
        uint16_t thirteen_imm = line & 8191;
        uint16_t seven_imm = line & 127;
        uint16_t four_imm = line & 15;
        uint16_t regSrcA = (line >> 10) & 7 ;
        uint16_t regSrcB = (line >> 7) & 7 ;
        uint16_t regSrcC = (line >> 4) & 7;

        if (opCode == 0) {
            //add, sub, or, and, slt, jr
            if (four_imm == 0) { //add
                if (regSrcC != R[0]) {
                    R[regSrcC] = R[regSrcA] + R[regSrcB];
                }
                pc++;
            } else if (four_imm == 1) { //sub
                if (regSrcC != R[0]) {
                    R[regSrcC] = R[regSrcA] - R[regSrcB];
                }
                pc++;
            } else if (four_imm == 2) { //or
                if (regSrcC != R[0]) {
                    R[regSrcC] = R[regSrcA] | R[regSrcB];
                }
                pc++;
            } else if (four_imm == 3) { //and
                if (regSrcC != R[0]) {
                    R[regSrcC] = R[regSrcA] & R[regSrcB];
                }
                pc++;
            } else if (four_imm == 4) { //slt
                if (regSrcC != R[0]) {
                    if (R[regSrcB] > R[regSrcA]) {R[regSrcC] = 1;}
                    else {R[regSrcC] = 0;}
                }
                pc++;
            } else if (four_imm == 8) { // jr
                pc = R[regSrcA];
            }

        } else if (opCode == 1) { //addi
            if (regSrcB != R[0]) {
                signExtend(seven_imm);
                R[regSrcB] = R[regSrcA] + seven_imm;
            }
            pc++;
        } else if (opCode == 2) { //j
            pc = thirteen_imm;

        } else if (opCode == 3) { //jal
            R[7] = pc + 1;
            pc = thirteen_imm;

        } else if (opCode == 4) { //lw
            cache_func((R[regSrcA] + seven_imm) % MEM_SIZE, position % 8192, cache, true);
            if (regSrcB != R[0]) {
                signExtend(seven_imm);
                R[regSrcB] =  memory[(R[regSrcA] + seven_imm) % MEM_SIZE];
            }
            pc++;
        } else if (opCode == 5) { //sw
            cache_func((R[regSrcA] + seven_imm) % MEM_SIZE, (position % 8192), cache, false);
            if(regSrcB != R[0]) {
                signExtend(seven_imm);
                memory[(R[regSrcA] + seven_imm) % MEM_SIZE] = R[regSrcB];
                pc++;
            }
        } else if (opCode == 6) { //jeq
            signExtend(seven_imm);
            if (R[regSrcA] == R[regSrcB]) {pc = pc+1+seven_imm;}
            else {pc++;}

        } else if (opCode == 7) { //slti
            if (regSrcB != R[0]) {
                signExtend(seven_imm);
                if (R[regSrcA] < seven_imm) {R[regSrcB] = 1;}
                else {R[regSrcB] = 0;}
            }
            pc++;
        }
        if (pc == position) halt = true;
    }

};


/*
    Prints out a correctly-formatted log entry.

    @param cache_name The name of the cache where the event
        occurred. "L1" or "L2"

    @param status The kind of cache event. "SW", "HIT", or
        "MISS"

    @param pc The program counter of the memory
        access instruction

    @param addr The memory address being accessed.

    @param row The cache row or set number where the data
        is stored.
*/
/*
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])

    @param num_rows The number of rows in the given cache.
*/

/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    string cache_config;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl;
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,associativity,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,associativity,blocksize,size,associativity,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    uint16_t pc = 0;
    uint16_t R[NUM_REGS] = {0};   // registers are initialized to 0
    uint16_t memory[MEM_SIZE] = {0}; // memory is initialized to 0
    load_machine_code(f, memory);


    /* parse cache config */
    if (cache_config.size() > 0) {
        vector<int> parts;
        size_t pos;
        size_t lastpos = 0;
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            // TODO: execute E20 program and simulate one cache here
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1size / (L1assoc * L1blocksize));

            Cache main_cache;
            Level L1_level(L1size, L1assoc, L1blocksize);
            main_cache.levels.push_back(L1_level);

            simulator(pc,  R,  memory, main_cache);

        } else if (parts.size() == 6) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int L2size = parts[3];
            int L2assoc = parts[4];
            int L2blocksize = parts[5];
            // TODO: execute E20 program and simulate two caches here
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1size / (L1assoc * L1blocksize));
            print_cache_config("L2", L2size, L2assoc, L2blocksize, L2size / (L2assoc * L2blocksize));

            Cache main_cache;
            Level L1_level(L1size, L1assoc, L1blocksize);
            Level L2_level(L2size, L2assoc, L2blocksize);
            main_cache.levels.push_back(L1_level);
            main_cache.levels.push_back(L2_level);

            simulator(pc,  R,  memory, main_cache);
        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }
    }

    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9


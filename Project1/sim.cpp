/*
CS-UY 2214
Adapted from Jeff Epstein
Starter code for E20 simulator
sim.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>
#include <cstdint>
//#include <bitset> 

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
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

void signExtend(uint16_t& number) {
    if ( (number >> 6) == 1) {
        number = number | 65408; 
    } 
}

void simulator(uint16_t& pc, uint16_t R[], uint16_t memory[]) {  
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
            if (regSrcB == R[0]) break; 
            signExtend(seven_imm); 
            R[regSrcB] = R[regSrcA] + seven_imm; 
            pc++;

        } else if (opCode == 2) { //j
            pc = thirteen_imm;

        } else if (opCode == 3) { //jal
            R[7] = pc + 1; 
            pc = thirteen_imm; 

        } else if (opCode == 4) { //lw
            if (regSrcB != R[0]) {
                signExtend(seven_imm); 
                R[regSrcB] =  memory[R[regSrcA] + seven_imm]; 
            }
            pc++;
        } else if (opCode == 5) { //sw
            signExtend(seven_imm); 
            memory[R[regSrcA] + seven_imm] = R[regSrcB]; 
            pc++;

        } else if (opCode == 6) { //jeq
            signExtend(seven_imm);
            if (R[regSrcA] == R[regSrcB]) {pc = pc+1+seven_imm;}
            else {pc++;}

        } else if (opCode == 7) { //slti 
            if (regSrcB != R[0]) {
                if (R[regSrcA] < seven_imm) {R[regSrcB] = 1;}
                else {R[regSrcB] = 0;}
            }
            pc++; 
        }
        if (pc == position) halt = true; 
    }

}

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
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
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
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // TODO: your code here. Load f and parse using load_machine_code
    uint16_t pc = 0;
    uint16_t R[NUM_REGS] = {0};   // registers are initialized to 0
    uint16_t memory[MEM_SIZE] = {0}; // memory is initialized to 0
    load_machine_code(f, memory);

    simulator(pc, R, memory);
    
    // TODO: your code here. print the final state of the simulator before ending, using print_state
    print_state(pc, R, memory, 128);
    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9

//cout << bitset<16>(line) << endl; 

/*
CS-UY 2214
Alma Marcu
simcache.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <list>
#include <iomanip>
#include <regex>
#include <cstdlib>
#include <cstdint>
using namespace std;
//constants from last time - registers and memory sizes
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

int ram[REG_SIZE] = {0};
const int REGISTERS = 8;
uint16_t registers[REGISTERS] = {0};
size_t code_section_end;
// row class (a single cache row) 
class Row {
private:
    list<int> list_lru; // tracks usage order (least recently used blocks)
    int assoct; // associativity of row (number of blocks per set)
public:
    Row(int assoct) : assoct(assoct) {}
    bool parse_block(int tag) { // checks if tag exists, updates LRU if it is hit
        for (auto i = list_lru.begin(); i != list_lru.end(); i++) {
            if (*i == tag) {
                list_lru.erase(i);
                list_lru.push_back(tag);
                return true; // returns true if HIT, false if a MISS
            }
        }
        return false;
    }
    void move_to_latest_block(int tag) { //moves block to most recent place
       parse_block(tag); // calls parse_block 
    }
    bool direct(int tag) { // accesses direct-mapped
        bool hit = (!list_lru.empty() && list_lru.front() == tag);
        list_lru.clear(); //clears lru
        list_lru.push_back(tag); //pushes back the tag to reset
        return hit;
    }
    //inserts a new block 
    void set(int tag) {
        if (list_lru.size() >= (size_t)assoct) {
            list_lru.pop_front(); // evicts if filled
        }
        list_lru.push_back(tag);
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
*/ // given to us
void print_log_entry(const string &cache_name, const string &status, int pc, int addr, int row) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
         " pc:" << setw(5) << pc <<
         "\taddr:" << setw(5) << addr <<
         "\trow:" << setw(4) << row << endl;
}
// cache class for L1 and L2
class Cache {
private:// holds the rows, size of cache, asst, block_size, and num of rows along with the cache name
    int size, asst, block_size, num_of_rows;
    vector<Row> rows;
    string cache;
public:
//cache constructor 
    Cache(int size, int asst, int block_size, string cache) : size(size), asst(asst), block_size(block_size), cache(cache) {
        num_of_rows = size / (asst*block_size);
        //calculates number of rows  and fills the rows with the associativiy
        for (int i = 0; i < num_of_rows; ++i) {
            rows.emplace_back(asst);
        }
    }
    //accesses the memory and returns HIT or MISS so it can be outputted 
    string access(int address, int pc) {
        //calculates block_id, row, and tag everytime memory is accessed
        int block_id = address / block_size ;
        int row = block_id % num_of_rows;
        int tag = block_id / num_of_rows;
        string update;
        //if the associativity is 1, the update to be returned is HIT if the tag exists/matches.. and MISS otherwise
        if (asst == 1) {
            update = rows[row].direct(tag) ? "HIT" : "MISS";
        } else {
            if (!rows[row].parse_block(tag)) {
                update = "MISS";
                rows[row].set(tag);
            } else {
                update = "HIT";
                rows[row].move_to_latest_block(tag);
            }
        }
        //access print log entry
        print_log_entry(cache, update, pc, address, row);
        return update;
    }
    //gets the row that the address maps to
    int get_row(int address) {
        int block_id = address / block_size;
        return block_id % num_of_rows;
    }
    //writes to cache
    void write(int address, int pc) {
        //calculates block_id, row, and tag everytime memory is accessed
        int block_id = address / block_size ;
        int row = block_id % num_of_rows;
        int tag = block_id / num_of_rows;
        //calls direct is associativity = 1
        if (asst == 1) {
            rows[row].direct(tag);
        } else {
            if (rows[row].parse_block(tag)) {} 
            rows[row].set(tag);
        }
        print_log_entry(cache, "SW", pc, address, row);
    }
    // inserts the block without making a log
    void insert(int address, int pc) {
        //calculates block_id, row, and tag everytime memory is accessed
        int block_id = address / block_size;
        int row = block_id % num_of_rows;
        int tag = block_id / num_of_rows;
        if (asst == 1) {
            rows[row].direct(tag);
        } else if (!rows[row].parse_block(tag)) {
            rows[row].set(tag);
        }
    }


};
/*
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])

    @param num_rows The number of rows in the given cache.
*/
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
         ", associativity " << assoc << ", blocksize " << blocksize <<
         ", rows " << num_rows << endl;
}


// given to me from last time
void load_machine_code(ifstream &f, unsigned mem[]) {
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
    code_section_end = expectedaddr;  // tracks the end of loaded code

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
//given to me
void print_state(unsigned pc, uint16_t regs[], unsigned memory[], size_t memquantity) {
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
//my simulate function from project 1 edited... runs with cache layers
//now it takes cache pointers to L1 and L2
//since L2 does 
void simulate(unsigned& pc, unsigned memory[], Cache* L1, Cache* L2 = nullptr) {
    bool check = true;
    while (check) {
        if (pc >= MEM_SIZE) {
            pc = pc % MEM_SIZE;
        }
        uint16_t instruction = memory[pc];
        int opcode = (instruction >> 13) & 0b111;

        // jump instructions
        if (opcode == 0b010 || opcode == 0b011) {
            unsigned int address = instruction & 0b1111111111111;
            if (opcode == 0b010) { // j
                if (address == pc) {
                    check = false; // ends the loop
                } else {
                    pc = address;
                    pc = pc % MEM_SIZE;
                }
            } else { // jal
                registers[7] = pc + 1; // saves return address in $7
                pc = address;
                pc = pc % MEM_SIZE;

            }
        }
            
        else if (opcode == 0b000) {
            //
            int rd = (instruction >> 10) & 0b111;
            int rs = (instruction >> 7) & 0b111;
            int rt = (instruction >> 4) & 0b111;
            if ((instruction & 0b1111) == 0) {
                registers[rt] = registers[rs] + registers[rd];
                pc = (pc + 1) % MEM_SIZE;
            } // add
            else if ((instruction & 0b1111) == 1) {
                registers[rt] = registers[rd] - registers[rs];
                pc = (pc + 1) % MEM_SIZE;
            }// sub
            else if ((instruction & 0b1111) == 2) {
                registers[rt] = registers[rs] | registers[rd];
                pc = (pc + 1) % MEM_SIZE;
            }// or
            else if ((instruction & 0b1111) == 3) {
                registers[rt] = registers[rs] & registers[rd];
                pc = (pc + 1) % MEM_SIZE;
            }// and
            else if ((instruction & 0b1111) == 4) {
                if (registers[rs] < registers[rt]) {
                    registers[rd] = 1;
                } else {
                    registers[rd] = 0;
                } // slt
                pc = (pc + 1) % MEM_SIZE;
            } else if ((instruction & 0b1111) == 8) {
                pc = registers[rs] % MEM_SIZE;
            } // jr
        }
        else if (opcode == 0b100 || opcode == 0b111 || opcode == 0b101 ||
                 opcode == 0b110 || opcode == 0b001) {

            int rs = (instruction >> 10) & 0b111;
            int rd = (instruction >> 7) & 0b111;
            int imm = instruction & 0b1111111;
            if (imm & 0b1000000) {
                imm |= 0b1111111111000000;
            }
            if (opcode == 0b111) {
                int16_t rs_val = static_cast<int16_t>(registers[rs]);
                int16_t imm_val = static_cast<int16_t>(imm);
                if (rs_val < imm_val) {
                    registers[rd] = 1;
                } else {
                    registers[rd] = 0;
                }
                pc = (pc + 1) % MEM_SIZE;
                // slti
            // sw has been changed to account for accessing the cache memory
            // i store the address of registers[rs] + imm and then update the 
            // memory at address to equal the register rd's value
            // then i write to L1 using the address and pc value
            // if L2 exists, then i also write to L2 to access the memory there as well
            // finally the pc is incremented
            } else if (opcode == 0b101) { // sw
                int addr = (registers[rs] + imm) % MEM_SIZE;
                // memory[(registers[rs] + imm) % MEM_SIZE] = registers[rd];
                memory[addr] = registers[rd];
                L1->write(addr, pc);
                if (L2) {
                    L2->write(addr, pc);
                }
                pc = (pc + 1) % MEM_SIZE;
            }
            else if (opcode == 0b001) { // addi
                // check if regdst = 0
                registers[rd] = (registers[rs] + imm);
                pc = (pc + 1) % MEM_SIZE;
            // lw has also been changed to access cache memory. address is yet again stored
            // the status of L1 being either a HIT or MISS is checked after being accessed. 
            // if the L1 status is a MISS and L2 does exist, the memory of L2 is then accessed
            // if L2 doesnt exist or L1's status is a HIT, the the register's rd value is updated
            // to the new memory's address. pc is then incremented
            } else if (opcode == 0b100) { //lw
                int addr = (registers[rs] + imm) % MEM_SIZE;
                string L1_status = L1->access(addr, pc);
                if (L1_status == "MISS" && L2) {
                    L2->access(addr, pc);
                }
                registers[rd] = memory[addr];
                pc = (pc + 1) % MEM_SIZE;

            } 
            else { // jeq
                if (registers[rs] == registers[rd]) {
                    pc = (pc + imm + 1)% MEM_SIZE;
                } else {
                    pc = (pc + 1) % MEM_SIZE;
                }
            }
        }
        registers[0] = 0; // makes sure that register 0 is never altered
        if (pc == MEM_SIZE - 1) {
            cout << "Program terminated normally." << endl;
            check = false;  // exits if last memory used
        }
        



    }
}


/**
    Main function
    Takes command-line args as documented below
*/
//given to me
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

        ifstream file(filename);
        if (!file) {
            cerr << "Cannot open file." << endl;
            return 1;
        }
        unsigned memory[MEM_SIZE] =  {0};
       // calling load machine code function here
        load_machine_code(file, memory);
       // initializing L1 and L2 caches to nullptrs
        Cache* L1 = nullptr;
        Cache* L2 = nullptr;

        if (parts.size() == 3) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            // TODO: execute E20 program and simulate one cache here
            // L1 is created using the values passed in and given the cache name L1

            L1 = new Cache(L1size, L1assoc, L1blocksize, "L1");
            // print cache config is called to output the values first passed in
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1size / (L1assoc * L1blocksize));
            // pc starts at 0, and then is passed in with the memory and L1 into simulate
            unsigned pc = 0;
            simulate(pc, memory, L1);

        } else if (parts.size() == 6) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int L2size = parts[3];
            int L2assoc = parts[4];
            int L2blocksize = parts[5];
            // TODO: execute E20 program and simulate two caches here
            
            // L1 is created using the values passed in and given the cache name L1
            // L2 is created using the values passed in and given the cache name L2

            L1 = new Cache(L1size, L1assoc, L1blocksize, "L1");
            L2 = new Cache(L2size, L2assoc, L2blocksize, "L2");
            // print cache config is called to output the values first passed in for each
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1size / (L1assoc * L1blocksize));
            print_cache_config("L2", L2size, L2assoc, L2blocksize, L2size / (L2assoc * L2blocksize));
// pc starts at 0, and then is passed in with the memory and L1 and L2 into simulate
           
            unsigned pc = 0;
            simulate(pc, memory, L1, L2);


        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }

        delete L1;
        delete L2;
    }

    return 0;
}
// Some helpful constant values that we'll be using.

/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/



//ra0Eequ6ucie6Jei0koh6phishohm9

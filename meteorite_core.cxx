#include "Supernova/supernova.h"
#include "meteorite_core.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cctype>
#include <algorithm>

// map of all possible instructions
std::unordered_map<std::string, supernova::inspx> const instruction_map = {
    {"andr", supernova::inspx::andr_instrc},
    {"andi", supernova::inspx::andi_instrc},
    {"xorr", supernova::inspx::xorr_instrc},
    {"xori", supernova::inspx::xori_instrc},
    {"orr", supernova::inspx::orr_instrc},
    {"ori", supernova::inspx::ori_instrc},
    {"not", supernova::inspx::not_instrc},
    {"cnt", supernova::inspx::cnt_instrc},
    {"llsr", supernova::inspx::llsr_instrc},
    {"llsi", supernova::inspx::llsi_instrc},
    {"lrsr", supernova::inspx::lrsr_instrc},
    {"lrsi", supernova::inspx::lrsi_instrc},
    {"addr", supernova::inspx::addr_instrc},
    {"addi", supernova::inspx::addi_instrc},
    {"subr", supernova::inspx::subr_instrc},
    {"subi", supernova::inspx::subi_instrc},
    {"umulr", supernova::inspx::umulr_instrc},
    {"umuli", supernova::inspx::umuli_instrc},
    {"smulr", supernova::inspx::smulr_instrc},
    {"smuli", supernova::inspx::smuli_instrc},
    {"udivr", supernova::inspx::udivr_instrc},
    {"udivi", supernova::inspx::udivi_instrc},
    {"sdivr", supernova::inspx::sdivr_instrc},
    {"sdivi", supernova::inspx::sdivi_instrc},
    {"call", supernova::inspx::call_instrc},
    {"push", supernova::inspx::push_instrc},
    {"retn", supernova::inspx::retn_instrc},
    {"pull", supernova::inspx::pull_instrc},
    {"ldb", supernova::inspx::ld_byte_instrc},
    {"ldh", supernova::inspx::ld_half_instrc},
    {"ldw", supernova::inspx::ld_word_instrc},
    {"ldd", supernova::inspx::ld_dwrd_instrc},
    {"stb", supernova::inspx::st_byte_instrc},
    {"sth", supernova::inspx::st_half_instrc},
    {"stw", supernova::inspx::st_word_instrc},
    {"std", supernova::inspx::st_dwrd_instrc},
    {"jal", supernova::inspx::jal_instrc},
    {"jalr", supernova::inspx::jalr_instrc},
    {"je", supernova::inspx::je_instrc},
    {"jne", supernova::inspx::jne_instrc},
    {"jgu", supernova::inspx::jgu_instrc},
    {"jgs", supernova::inspx::jgs_instrc},
    {"jleu", supernova::inspx::jleu_instrc},
    {"jles", supernova::inspx::jles_instrc},
    {"setgur", supernova::inspx::setgur_instrc},
    {"setgui", supernova::inspx::setgui_instrc},
    {"setgsr", supernova::inspx::setgsr_instrc},
    {"setgsi", supernova::inspx::setgsi_instrc},
    {"setleur", supernova::inspx::setleur_instrc},
    {"setleui", supernova::inspx::setleui_instrc},
    {"setlesr", supernova::inspx::setlesr_instrc},
    {"setlesi", supernova::inspx::setlesi_instrc},
    {"lui", supernova::inspx::lui_instrc},
    {"auipc", supernova::inspx::auipc_instrc},
    {"pcall", supernova::inspx::pcall_instrc},
    {"outb", supernova::inspx::bout_instrc},
    {"outw", supernova::inspx::out_instrc},
    {"inb", supernova::inspx::bin_instrc},
    {"inw", supernova::inspx::in_instrc}};

/**
 * @brief trim trailing whitespace and remove comments on a line
*/
inline std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), isspace));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), isspace).base(), s.end());
    s.erase(std::find(s.begin(), s.end(), ';'), s.end());
    return s;
}


struct later_label {
    /** name of the label */
    std::string name;
    /** index on the instructions */
    uint64_t index;
    
    uint8_t rd;
    uint8_t r1;
    supernova::inspx opcode;
    bool is_l_type;
};

bool match_label_expr(const std::string &str, std::string &label) {
    if (str.size() > 34 || str.front() != '[' || str.back() != ':' || *(str.end() - 2) != ']')
        return false;
    label = str.substr(1, str.size() - 3);
    return true;
}

bool match_rinst_expr(const std::string &str, std::string &opcode, uint8_t &rd, uint8_t &r1, uint8_t &r2) {
    size_t pos1 = str.find(" r");
    if (pos1 == std::string::npos) return false;
    pos1++;
    opcode = trim(str.substr(0, pos1));
    size_t pos2 = str.find('r', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    size_t pos3 = str.find('r', pos2 + 1);
    if (pos3 == std::string::npos) return false;
    rd = std::stoi(str.substr(pos1 + 1, pos2 - pos1 - 1));
    r1 = std::stoi(str.substr(pos2 + 1, pos3 - pos2 - 1));
    r2 = std::stoi(str.substr(pos3 + 1));
    return true;
}

bool match_sinst_expr(const std::string &str, std::string &opcode, uint8_t &rd, uint8_t &r1, uint64_t &immediate, std::string &label) {
    size_t pos1 = str.find(" r");
    if (pos1 == std::string::npos) return false;
    pos1++;
    opcode = trim(str.substr(0, pos1));
    size_t pos2 = str.find('r', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    size_t pos3 = str.find(' ', pos2 + 1);
    if (pos3 == std::string::npos) return false;
    rd = std::stoi(str.substr(pos1 + 1, pos2 - pos1 - 1));
    r1 = std::stoi(str.substr(pos2 + 1, pos3 - pos2 - 1));
    std::string imm_label = str.substr(pos3 + 1);
    if (imm_label.front() == '$') {
        immediate = std::stoul(imm_label.substr(1));
    } else if (imm_label.front() == '#') {
        immediate = std::stoul(imm_label.substr(1), nullptr, 16);
    } else if (imm_label.front() == '[' && imm_label.back() == ']') {
        label = imm_label.substr(1, imm_label.size() - 2);
    } else {
        return false;
    }
    return true;
}

bool match_linst_expr(const std::string &str, std::string &opcode, uint8_t &rd, uint64_t &immediate, std::string &label) {
    size_t pos1 = str.find(" r");
    if (pos1 == std::string::npos) return false;
    pos1++;
    opcode = trim(str.substr(0, pos1));
    size_t pos2 = str.find(' ', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    rd = std::stoi(str.substr(pos1 + 1, pos2 - pos1 - 1));
    std::string imm_label = str.substr(pos2 + 1);
    if (imm_label.front() == '$') {
        immediate = std::stoul(imm_label.substr(1));
    } else if (imm_label.front() == '#') {
        immediate = std::stoul(imm_label.substr(1), nullptr, 16);
    } else if (imm_label.front() == '[' && imm_label.back() == ']') {
        label = imm_label.substr(1, imm_label.size() - 2);
    } else {
        return false;
    }
    return true;
}

uint64_t convert_instructions(std::stringstream &codestream, std::vector<uint64_t> & instructions)
{
    const char name[6]{0};
    char label_v[33]{0};
    uint8_t rd = 0;
    uint8_t r1 = 0;
    uint8_t r2 = 0;
    uint64_t immediate{0};

    std::string instruction;
    
    std::unordered_map<std::string, uint64_t> label_map;
    std::vector<later_label> undefined_labels;
    uint64_t instruction_pointer = 0;
    uint64_t binstr{0};


    while (std::getline(codestream, instruction))
    {
        instruction = trim(instruction);
        if (instruction.empty())
        {
            continue;
        }

        auto inststr = instruction;
        std::string opcode, label;
        if (match_label_expr(inststr, label)) {
            label_map[label] = instruction_pointer;
            continue;
        }
        if (match_rinst_expr(inststr, opcode, rd, r1, r2)) {
            auto const opcode_enum = instruction_map.at(opcode);
            binstr = static_cast<uint64_t>(supernova::RInstruction(opcode_enum, r1, r2, rd));
        }
        else if (match_sinst_expr(inststr, opcode, rd, r1, immediate, label)) {
            auto opcode_enum = instruction_map.at(opcode);
            if (label.empty()) {
                binstr = static_cast<uint64_t>(supernova::SInstruction(opcode_enum, r1, rd, immediate));
            } else {
                if (!label_map.contains(label)) {
                    undefined_labels.emplace_back(
                        label, instruction_pointer / 8, rd, r1, opcode_enum, false
                    );
                    binstr = 0;
                } else {
                    auto address = label_map[label];
                    auto reladd = address - instruction_pointer;
                    binstr = static_cast<uint64_t>(supernova::SInstruction(opcode_enum, r1, rd, reladd));
                }
            }
        }
        else if (match_linst_expr(inststr, opcode, rd, immediate, label)) {
            auto opcode_enum = instruction_map.at(opcode);
            if (label.empty()) {
                binstr = static_cast<uint64_t>(supernova::LInstruction(opcode_enum, rd, immediate));
            } else {
                if (!label_map.contains(label)) {
                    undefined_labels.emplace_back(
                        label, instruction_pointer / 8, rd, r1, opcode_enum, true
                    );
                    binstr = 0;
                } else {
                    auto address = label_map[label];
                    auto reladd = address - instruction_pointer;
                    binstr = static_cast<uint64_t>(supernova::SInstruction(opcode_enum, r1, rd, reladd));
                }
            }
        } else {
            return 1;
        }
        
        instructions.push_back(binstr);

        instruction_pointer += 8;
    }

    for (auto &&undefined : undefined_labels)
    {
        if (!label_map.contains(undefined.name)) {
            return 2;
        }

        uint64_t const reladd = label_map[undefined.name] - (undefined.index * 8);
        if (undefined.is_l_type) {
            instructions[undefined.index] = static_cast<uint64_t>(supernova::LInstruction(undefined.opcode, undefined.rd, reladd));
        } else {
            instructions[undefined.index] = static_cast<uint64_t>(supernova::SInstruction(undefined.opcode, undefined.r1, undefined.rd, reladd));
        }
    }

    return 0;    
}

using namespace supernova::headers;

void meteorite::generate_code(char * filename) {
    std::ifstream file(filename);
    std::stringstream strstr{};
    file >> strstr.rdbuf();

    std::vector<uint64_t> instructions{};
    auto result = convert_instructions(strstr, instructions);
    if (result == 1) {
        std::cout << "invalid instruction\n";
        return;
    }

    if (result == 2) {
        std::cout << "label does not exist\n";
        return;
    }

    auto output = std::string(filename) + ".mtr";

    main_header header {
        .magic = master_magic,
        .version = snvm_version,
        .memory_size = 1 << 24,
        .entry_point = 0,
        .memory_regions = 1
    };

    memory_map mmap {
        .magic = memmap_magic,
        .start = sizeof(main_header) + sizeof(memory_map),
        .size = instructions.size() * sizeof(uint64_t),
        .offset = 0x1000,
        .flags = static_cast<memory_flags>(mem_read | mem_execute | mem_exists)
    };

    auto outfile = std::fstream(output, std::ios::out | std::ios::binary);
    outfile.write(reinterpret_cast<char*>(&header), sizeof(header));
    outfile.write(reinterpret_cast<char*>(&mmap), sizeof(mmap));
    outfile.write(reinterpret_cast<char*>(&instructions.front()), sizeof(uint64_t) * instructions.size());
}
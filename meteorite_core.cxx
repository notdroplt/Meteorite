#include "Supernova/supernova.h"
#include "meteorite_core.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <regex>

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

inline void trim(std::string &s)
{
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), isspace));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), isspace).base(), s.end());
    s.erase(std::find(s.begin(), s.end(), ';'), s.end());
}

struct later_label {
    std::string name;
    uint64_t index;
    uint8_t rd;
    uint8_t r1;
    supernova::inspx opcode;
    bool is_l_type;
};
inline auto const label_expr =       std::regex(R"-(\[(.{1,32})\]:)-");
inline auto const rinst_expr =       std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+r(\d{1,2})\s+r(\d{2}))-");
inline auto const sinst_label_expr = std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+r(\d{1,2})\s+\[(.{1,32})\])-");
inline auto const sinst_dimm_expr =  std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+r(\d{1,2})\s+\$(\d+))-");
inline auto const sinst_ximm_expr =  std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+r(\d{1,2})\s+#([0-9a-fA-F]+))-");
inline auto const linst_label_expr = std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+\[(.{1,32})\])-");
inline auto const linst_dimm_expr =  std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+\$(\d+))-");
inline auto const linst_ximm_expr =  std::regex(R"-((\w{3,5})\s+r(\d{1,2})\s+#([0-9a-fA-F]+))-");
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
        trim(instruction);
        if (instruction.empty())
        {
            continue;
        }

        auto inststr = instruction;
        std::smatch regex_match;
        if (std::regex_match(inststr, regex_match, label_expr) && regex_match.size() == 2)
        {
            label_map[regex_match[1]] = instruction_pointer;
            continue;
        }
        if (std::regex_match(inststr, regex_match, rinst_expr) && regex_match.size() == 5)
        {
            auto const opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            r1 = std::stoi(regex_match[3]);
            r2 = std::stoi(regex_match[4]);
            binstr = static_cast<uint64_t>(supernova::RInstruction(opcode, r1, r2, rd));
        }
        else if (std::regex_match(inststr, regex_match, sinst_ximm_expr) && regex_match.size() == 5)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            r1 = std::stoi(regex_match[3]);
            immediate = std::stol(regex_match[4], nullptr, 16);
            binstr = static_cast<uint64_t>(supernova::SInstruction(opcode, r1, rd, immediate));
        }
        else if (std::regex_match(inststr, regex_match, sinst_dimm_expr) && regex_match.size() == 5)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            r1 = std::stoi(regex_match[3]);
            immediate = std::stol(regex_match[4]);
            binstr = static_cast<uint64_t>(supernova::SInstruction(opcode, r1, rd, immediate));
        }
        else if (std::regex_match(inststr, regex_match, sinst_label_expr) && regex_match.size() == 5)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            r1 = std::stoi(regex_match[3]);
            if (!label_map.contains(regex_match[4])) {
                undefined_labels.emplace_back(
                    label_v, instruction_pointer / 8, rd, r1, opcode, false
                );
                binstr = 0;
            } else {
                auto address = label_map[regex_match[4]];
                auto reladd = address - instruction_pointer;
                binstr = static_cast<uint64_t>(supernova::SInstruction(opcode, r1, rd, reladd));
            }
        }
        else if (std::regex_match(inststr, regex_match, linst_ximm_expr) && regex_match.size() == 4)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            immediate = std::stol(regex_match[3], nullptr, 16);
            binstr = static_cast<uint64_t>(supernova::LInstruction(opcode, rd, immediate));
        }
        else if (std::regex_match(inststr, regex_match, linst_dimm_expr) && regex_match.size() == 4)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            immediate = std::stol(regex_match[3]);
            binstr = static_cast<uint64_t>(supernova::LInstruction(opcode, r1, immediate));
        }
        else if (std::regex_match(inststr, regex_match, linst_label_expr) && regex_match.size() == 4)
        {
            auto opcode = instruction_map.at(regex_match[1]);
            rd = std::stoi(regex_match[2]);
            if (!label_map.contains(regex_match[3])) {
                undefined_labels.emplace_back(
                    label_v, instruction_pointer / 8, rd, r1, opcode, false
                );
                binstr = 0;
            } else {
                auto address = label_map[regex_match[3]];
                auto reladd = address - instruction_pointer;
                binstr = static_cast<uint64_t>(supernova::SInstruction(opcode, r1, rd, reladd));
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

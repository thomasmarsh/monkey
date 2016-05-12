#pragma once

#include "mcts_node.h"

template <typename N, typename Ptr=typename N::Ptr>
struct DotWriter {
    FILE *fp;

    DotWriter(Ptr root, size_t move_count) {
        root->sort();

        auto fname = fmt::format("ismcts/ismcts.{}", move_count);

        fp = fopen(fname.c_str(), "w");
        assert(fp);

        fprintf(fp, "digraph G {\n");
        fprintf(fp, "  rankdir = LR;\n");

        int n = 0;
        writeNodeDefs(root, n);
        n = 0;
        writeNodeConnections(root, n);
        fprintf(fp, "}\n");
        fclose(fp);
    }

    std::string label(const Move &m) {
        return fmt::format("{} {} {}",
                           to_string(m.action()),
                           m.card() != CardRef(-1) ?  to_string(Card::Get(m.card())) : "null",
                           m.arg()  != Move::null  ?  std::to_string(m.arg()) : "null");
    }

    void writeNodeDefs(Ptr node, int &n, int indent=1) {
        int m = n;
        fprintf(fp, "%*cn%d [\n", indent*2, ' ', m);
        fprintf(fp, "%*c  label = \"%s | %s\"\n",
                indent*2, ' ',
                node->shortRepr().c_str(),
                EscapeQuotes(label(node->move)).c_str());
        fprintf(fp, "%*c  shape = record\n", indent*2, ' ');
        fprintf(fp, "%*c  style = filled\n", indent*2, ' ');
        std::string color;
        switch (node->just_moved) {
        case 0: color = "fc1e33"; break;
        case 1: color = "ff911e"; break;
        case 2: color = "21abd8"; break;
        case 3: color = "40ee1c"; break;
        }
        if (!color.empty()) {
            fprintf(fp, "%*c  fillcolor = \"#%s\"\n", indent*2, ' ', color.c_str());
        }
        fprintf(fp, "%*c];\n", indent*2, ' ');

        for (auto &child : node->children) {
            ++n;
            writeNodeDefs(child, n, indent+1);
        }
    }

    void writeNodeConnections(Ptr node, int &n) {
        int m = n;
        for (auto &child : node->children) {
            ++n;
            fprintf(fp, "  n%d -> n%d;\n", m, n);
            writeNodeConnections(child, n);
        }
    }
};

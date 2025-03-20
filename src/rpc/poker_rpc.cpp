// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker.h>
#include <rpc/server.h>
#include <rpc/server_util.h>

using node::NodeContext;

static std::string renderCards(const std::vector<Card>& cards) {
    std::vector<std::string> cardLines(7);

    for (const auto& card : cards) {
        std::string cardStr = card.toString();
        std::string rank, suit;

        if (cardStr.size() >= 4 && (unsigned char)cardStr[cardStr.size() - 3] >= 0xE2) {
            suit = cardStr.substr(cardStr.size() - 3);
            rank = cardStr.substr(0, cardStr.size() - 3);
        } else {
            suit = cardStr.substr(cardStr.size() - 1);
            rank = cardStr.substr(0, cardStr.size() - 1);
        }

        bool isTwoChar = (rank.size() == 2);
        std::string topRank    = isTwoChar ? rank + "      " : rank + "       ";
        std::string bottomRank = isTwoChar ? "      " + rank : "       " + rank;

        cardLines[0] += "┌─────────┐";
        cardLines[1] += "│ " + topRank + "│";
        cardLines[2] += "│         │";
        cardLines[3] += "│    " + suit + "    │";
        cardLines[4] += "│         │";
        cardLines[5] += "│" + bottomRank + " │";
        cardLines[6] += "└─────────┘";
    }

    return std::accumulate(cardLines.begin(), cardLines.end(), std::string(),
        [](const std::string& a, const std::string& b) {
            return a + b + "\n";
        });
}

static std::string renderHiddenCards() {
    std::vector<std::string> cardLines(7);

    for (int i = 0; i < 3; i++) {
        cardLines[0] += "┌─────────┐";
        cardLines[1] += "│ ******* │";
        cardLines[2] += "│ ******* │";
        cardLines[3] += "│ ******* │"; 
        cardLines[4] += "│ ******* │";
        cardLines[5] += "│ ******* │";
        cardLines[6] += "└─────────┘";
    }

    return std::accumulate(cardLines.begin(), cardLines.end(), std::string(),
        [](const std::string& a, const std::string& b) {
            return a + b + "\n";
        });
}

static UniValue handleDeal(NodeContext &node) {
    auto& poker_worker = node.poker_worker;
    if (poker_worker->deck.size() < 3){
        throw JSONRPCError(RPC_MISC_ERROR, "Not enough cards to deal. Wait for the next block!");
    }
    if (!poker_worker->playerHand.empty()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Player cards are already dealt");
    }
    
    poker_worker->DealCards();
    std::string block_hash = poker_worker->currentBlockHash;
    int round = poker_worker->round;
    std::string cards = "[Poker Game Details]\n[Block: " + block_hash + "]\n"
                            + "[Round: " + std::to_string(round) + "]\n\n"
                            + "Your Hand:\n" 
                            + renderCards(poker_worker->playerHand)
                            + "Satoshi's Hand:\n"
                            + renderHiddenCards() + "\n"
                            + "Do you want to play or fold? (use rpc commands: 'poker play' or 'poker fold') \n";
    return UniValue(cards);
}

static UniValue handlePlay(NodeContext &node) {
    auto& poker_worker = node.poker_worker;
    if (poker_worker->playerHand.empty()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Player cards are not dealt! (Or a new block has been mined)");
    }
    std::string block_hash = poker_worker->currentBlockHash;
    int player_score = poker_worker->EvaluateHand(poker_worker->playerHand);
    int satoshi_score = poker_worker->EvaluateHand(poker_worker->satoshisHand);
    int round = poker_worker->round;

    std::string cards = "[Poker Game Details]\n[Block: " + block_hash + "]\n"
                      + "[Round: " + std::to_string(round) + "]\n\n"
                      + "Your Hand: " + poker_worker->GetHandDescription(player_score) + "\n" 
                      + renderCards(poker_worker->playerHand)
                      + "Satoshi's Hand: " + poker_worker->GetHandDescription(satoshi_score) + "\n" 
                      + renderCards(poker_worker->satoshisHand);

    bool winner = player_score > satoshi_score;

    poker_worker->round++;
    poker_worker->playerHand.clear();
    poker_worker->satoshisHand.clear();

    if (winner) {
        cards += "Result: You won!\n";
    } else {
        cards += "Result: You lost!\n";
    }
    return UniValue(cards);
}

static UniValue handleFold(NodeContext &node) {
    auto& poker_worker = node.poker_worker;
    if (poker_worker->playerHand.empty()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Already folded, or cards not dealt");
    }
    poker_worker->round++;
    poker_worker->playerHand.clear();
    poker_worker->satoshisHand.clear();
    return UniValue("You folded!! (use rpc command: 'poker deal' to deal cards again)");
}

static RPCHelpMan poker()
{
    return RPCHelpMan{"poker",
        "Play a round of poker against your node! Use move 'deal', 'play', or 'fold'.",
        {
            {"move", RPCArg::Type::STR, RPCArg::Optional::NO, "The poker move 'deal', 'play', or 'fold'."}
        },
        RPCResult{
            RPCResult::Type::STR, "", "The result of the poker action."
        },
        RPCExamples{
            HelpExampleCli("poker", "\"deal\"")
            + HelpExampleRpc("poker", "\"deal\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string subcommand = request.params[0].get_str();
            NodeContext &node = EnsureAnyNodeContext(request.context);

            if (subcommand == "deal") {
                return handleDeal(node);
            } else if (subcommand == "play") {
                return handlePlay(node);
            } else if (subcommand == "fold") {
                return handleFold(node);
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown move, use 'deal', 'play', or 'fold'");
            }
        },
    };
}

void RegisterPokerRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"poker", &poker},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

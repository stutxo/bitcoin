// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker.h>
#include <chain.h>
#include <validation.h>
#include <random>
#include <algorithm>
#include <map>

bool PokerWorker::Init()
{
    LogPrintf("[poker] Initializing PokerWorker...\n");
    bool off = false;
    if (!m_running.compare_exchange_strong(off, true)) {
        LogPrintf("[poker] PokerWorker already running\n");
        return false;
    }
    std::thread([this] {
        util::ThreadRename(strprintf("pokerworker.%i", 0));
        LogPrintf("[poker] Starting PokerWorker processing thread\n");
        m_shutdown = true;
        m_shutdown.notify_one();
        LogPrintf("[poker] PokerWorker processing thread stopped\n");
    }).detach();
    LogPrintf("[poker] PokerWorker initialized successfully\n");
    return true;
}

void PokerWorker::WaitShutdown()
{
    m_shutdown.wait(false);
}

void PokerWorker::Stop()
{
    LogDebug(BCLog::POKER, "Stopping Poker Worker\n");
    m_running = false;
}

void PokerWorker::ActiveTipChange(const CBlockIndex& new_tip, bool is_ibd)
{
    AssertLockHeld(cs_main);

    if (is_ibd) return;
    if (!m_running) return;
    if (!m_node.wallet_loader) return;

    LOCK(m_deck_mutex);

    uint256 latest_block = new_tip.GetBlockHash();
    currentBlockHash = latest_block.GetHex();
    round = 1;
    playerHand.clear();
    satoshisHand.clear();
    GenerateDeck();
    ShuffleDeck(latest_block);
}

void PokerWorker::GenerateDeck() {
    deck.clear();
    for (int s = 0; s < 4; ++s) {
        for (int r = 2; r <= 14; ++r) {
            deck.push_back({static_cast<Rank>(r), static_cast<Suit>(s)});
        }
    }
}

void PokerWorker::ShuffleDeck(const uint256& blockHash) {
    std::string seedStr = blockHash.GetHex();
    std::seed_seq seed(seedStr.begin(), seedStr.end());
    std::mt19937 rng(seed);
    std::shuffle(deck.begin(), deck.end(), rng);
}

void PokerWorker::DealCards() {
    if (deck.size() < 6)
        throw std::runtime_error("Not enough cards to deal. Game over! Wait for the next block!");
    
    LOCK(m_deck_mutex);
    
    playerHand.clear();
    satoshisHand.clear();
    
    for (int i = 0; i < 3; i++) {
        playerHand.push_back(deck.back());
        deck.pop_back();
        
        satoshisHand.push_back(deck.back());
        deck.pop_back();
    }
}

int PokerWorker::EvaluateHand(const std::vector<Card>& hand) {
    std::vector<int> ranks;
    std::set<Suit> suits;

    for (const auto& card : hand) {
        ranks.push_back(static_cast<int>(card.rank));
        suits.insert(card.suit);
    }

    std::sort(ranks.begin(), ranks.end(), std::greater<int>());

    bool is_flush = suits.size() == 1;
    
    // Check for straight
    bool is_straight = false;
    if (ranks[0] - 1 == ranks[1] && ranks[1] - 1 == ranks[2]) {
        is_straight = true;
    }
    // Special case for A-2-3 (Ace is low)
    else if (ranks[0] == 14 && ranks[1] == 3 && ranks[2] == 2) { // Ace-3-2
        is_straight = true;
        // Move Ace to the back for proper low straight evaluation
        ranks = {3, 2, 1};  // Represent A-2-3 as 3-2-1 for scoring
    }

    // Hand rankings from highest to lowest:
    // 6: Straight Flush
    // 5: Three of a Kind
    // 4: Straight
    // 3: Flush
    // 2: Pair
    // 1: High Card
    
    int hand_rank = 1; // Default to high card
    
    // Evaluate hand type
    if (is_straight && is_flush) {
        hand_rank = 6; // Straight flush
    }
    else if (ranks[0] == ranks[1] && ranks[1] == ranks[2]) {
        hand_rank = 5; // Three of a kind
    }
    else if (is_straight) {
        hand_rank = 4; // Straight
    }
    else if (is_flush) {
        hand_rank = 3; // Flush
    }
    else if (ranks[0] == ranks[1] || ranks[1] == ranks[2]) {
        hand_rank = 2; // Pair
    }
    
    // Create a 32-bit integer representing the hand
    // First 4 bits: hand type (1-6)
    // Remaining bits: card ranks (4 bits each, up to 5 cards)
    int score = hand_rank << 28;
    
    // Add card ranks for tiebreakers
    if (hand_rank == 2) { 
        if (ranks[0] == ranks[1]) {
            score |= (ranks[0] << 24) | (ranks[2] << 20);
        } else {
            score |= (ranks[1] << 24) | (ranks[0] << 20);
        }
    } else {
        // For other hands, just use descending card ranks
        score |= (ranks[0] << 24) | (ranks[1] << 20) | (ranks[2] << 16);
    }
    
    return score;
}

std::string PokerWorker::GetHandDescription(int score) {
     int handType = (score >> 28) & 0xF;
    
     int card1 = (score >> 24) & 0xF;
     int card2 = (score >> 20) & 0xF;
     int card3 = (score >> 16) & 0xF;
     
     std::string handName;
     std::string description;
     
     auto rankToString = [](int rank) -> std::string {
         switch(rank) {
             case 14: return "A";
             case 13: return "K";
             case 12: return "Q";
             case 11: return "J";
             case 10: return "10";
             default: return std::to_string(rank);
         }
     };
     
     switch(handType) {
         case 6:
             handName = "Straight Flush";
             description = "high " + rankToString(card1);
             break;
         case 5:
             handName = "Three of a Kind";
             description = rankToString(card1) + "s";
             break;
         case 4:
             handName = "Straight";
             description = "high " + rankToString(card1);
             break;
         case 3:
             handName = "Flush";
             description = rankToString(card1) + "-" + rankToString(card2) + "-" + rankToString(card3);
             break;
         case 2:
             handName = "Pair";
             description = "of " + rankToString(card1) + "s with " + rankToString(card2) + " kicker";
             break;
         case 1:
             handName = "High Card";
             description = rankToString(card1) + "-" + rankToString(card2) + "-" + rankToString(card3);
             break;
         default:
             return "Unknown hand";
     }
     
     return handName + " (" + description + ")";
}
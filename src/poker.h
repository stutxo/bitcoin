#ifndef POKER_H
#define POKER_H

#include <validationinterface.h>
#include <node/context.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <atomic>
#include <sync.h>

enum class Suit { Hearts, Diamonds, Spades, Clubs };

enum class Rank { Two = 2, Three, Four, Five, Six, Seven, Eight,
                  Nine, Ten, Jack, Queen, King, Ace };

struct Card {
    Rank rank;
    Suit suit;

    std::string toString() const {
        static const std::map<Rank, std::string> rankStrings = {
            {Rank::Two, "2"}, {Rank::Three, "3"}, {Rank::Four, "4"},
            {Rank::Five, "5"}, {Rank::Six, "6"}, {Rank::Seven, "7"},
            {Rank::Eight, "8"}, {Rank::Nine, "9"}, {Rank::Ten, "10"},
            {Rank::Jack, "J"}, {Rank::Queen, "Q"}, {Rank::King, "K"},
            {Rank::Ace, "A"}
        };

        static const std::map<Suit, std::string> suitStrings = {
            {Suit::Hearts, "♥"}, {Suit::Diamonds, "♦"},
            {Suit::Spades, "♠"}, {Suit::Clubs, "♣"}
        };

        return rankStrings.at(rank) + suitStrings.at(suit);
    }
};

class PokerWorker final : public CValidationInterface
{
public:
    PokerWorker(node::NodeContext& node) : m_node(node) {}
    PokerWorker() = delete;
    ~PokerWorker()
    {
        Stop();
        WaitShutdown();
    }

    bool Init();
    void Stop();
    void WaitShutdown();
    void ActiveTipChange(const CBlockIndex& new_tip, bool is_ibd) final EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_deck_mutex);

    void GenerateDeck();
    void ShuffleDeck(const uint256& blockHash);
    void DealCards() EXCLUSIVE_LOCKS_REQUIRED(!m_deck_mutex);
    int EvaluateHand(const std::vector<Card>& hand);
    std::string GetHandDescription(int score);

    std::vector<Card> playerHand;
    std::vector<Card> satoshisHand;
    std::vector<Card> deck;
    std::string currentBlockHash;
    int round;

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdown{false};
    node::NodeContext& m_node;

    mutable Mutex m_deck_mutex;
};



#endif // POKER_H

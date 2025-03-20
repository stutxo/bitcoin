// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>

#include <string>

static RPCHelpMan playpoker()
{
    return RPCHelpMan{"playpoker",
        "Play a round of poker against your node!",
        {},
        RPCResult{
            RPCResult::Type::STR, "", ""
        },
        RPCExamples{
            HelpExampleCli("playpoker", "")
            + HelpExampleRpc("playpoker", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            return UniValue("poker!!");
        },
    };
}

void RegisterPokerRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"poker", &playpoker},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

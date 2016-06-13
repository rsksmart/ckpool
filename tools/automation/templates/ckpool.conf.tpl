{
    "btcd" : ${JSON.stringify(btcd)},
    "rskd" : ${JSON.stringify(rskd)},
    "upstream" : "main.ckpool.org:3336",
    "btcaddress" : "n19XNjTfYwLx3YbQk7GG2tLjzcqUeWP42D",
    "btcsig" : "/mined by rsk/",
    "blockpoll" : ${blockPoll},
    "nonce1length" : 4,
    "nonce2length" : 8,
    "update_interval" : 30,
    "serverurl" : ${JSON.stringify(serverUrl)},
    "nodeserver" : [
        "127.0.0.1:3335"
    ],
    "trusted" : [
        "127.0.0.1:3336"
    ],
    "mindiff" : 0,
    "startdiff" : 1,
    "maxdiff" : 0,
    "logdir" : "${logDir}",
    "rskpollperiod" : ${rskPollPeriod},
    "rsknotifypolicy": ${rskNotifyPolicy}
}
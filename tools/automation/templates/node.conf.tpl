blockchain.config.name = "regtest"

peer.discovery = {

    # if peer discovery is off
    # the peer window will show
    # only what retrieved by active
    # peer [true/false]
    enabled = false

    # number of workers that
    # tastes the peers for being
    # online [1..10]
    workers = 8

    # List of the peers to start
    # the search of the online peers
    # values: [ip:port, ip:port, ip:port ...]
    ip.list = [
    ]

    # indicates if the discovered nodes and their reputations
    # are stored in DB and persisted between VM restarts
    persist = false

    # the period in seconds with which the discovery
    # tries to reconnect to successful nodes
    # 0 means the nodes are not reconnected
    touchPeriod = 600

    # the maximum nuber of nodes to reconnect to
    # -1 for unlimited
    touchMaxNodes = 100

    # external IP/hostname which is reported as our host during discovery
    # if not set, the service http://checkip.amazonaws.com is used
    # the last resort is to get the peer.bind.ip address
    external.ip = null

    # indicates whether the discovery will include own home node
    # within the list of neighbor nodes
    public.home.node = true
}

peer {

    # Boot node list
    active = [
    ]

    # The protocols supported by peer
    # can be: [eth, shh, bzz]
    capabilities = [eth]

    # Local network adapter IP to which
    # the discovery UDP socket is bound
    # e.g: 192.168.1.104
    #
    # if the value is empty will be retrived
    # by punching to some know address e.g: www.google.com
    bind.ip = ""

    # Peer for server to listen for incoming
    # connections
    listen.port = 30305

    # connection timeout for trying to
    # connect to a peer [seconds]
    connection.timeout = 2

    # the parameter specifies how much
    # time we will wait for a message
    # to come before closing the channel
    channel.read.timeout = 30

    # Private key of the peer
    # derived nodeId = 2bc32aa570b3e292a9bd93380c1a23fb5c877d91bad5462affc80f45658e0abad0c4bef2d44aea6c2715e891114aa95ee09731dedf96bec099377a2f92aa13f0
    privateKey = bd3d20e480dfb1c9c07ba0bc8cf9052f89923d38b5128c5dbfc18d4eea3826af

    # Network id
    # networkId = 456123
    networkId = 456124

    p2p {
        # the default version outbound connections are made with
        # inbound connections are made with the version declared by the remote peer (if supported)
        # version = 4

        # max frame size in bytes when framing is enabled
        framing.maxSize = 32768

        # forces peer to send Handshake message in format defined by EIP-8,
        # see https://github.com/ethereum/EIPs/blob/master/EIPS/eip-8.md
        eip8 = true
    }

    # max number of active peers our node will maintain
    # extra peers trying to connect us will be dropeed with TOO_MANY_PEERS message
    # the incoming connection from the peer matching 'peer.trusted' entry is always accepted
    maxAcivePeers = 30
}

# the folder resources/genesis
# contains several versions of
# genesis configuration according
# to the network the peer will run on
genesis = rootstock-dev.json

# the time we wait to the network
# to approve the transaction, the
# transaction got approved when
# include into a transactions msg
# retrieved from the peer [seconds]
transaction.approve.timeout = 15

# the number of blocks should pass
# before pending transaction is removed
transaction.outdated.threshold = 10

# default directory where we keep
# basic Serpent samples relative
# to home.dir
samples.dir = samples

database {
    # place to save physical storage files
    dir = node1

    # every time the application starts
    # the existing database will be
    # destroyed and all the data will be
    # downloaded from peers again [true/false]
    reset = true
}

# this string is computed
# to be eventually the address
# that get the miner reward
coinbase.secret = monkey1

dump {
    # for testing purposes
    # all the state will be dumped
    # in JSON form to [dump.dir]
    # if [dump.full] = true
    # possible values [true/false]
    full = false
    dir = dmp

    # This defines the vmtrace dump
    # to the console and the style
    # -1 for no block trace
    # styles: [pretty/standard+] (default: standard+)
    block = -1
    style = pretty

    # clean the dump dir each start
    clean.on.restart = true
}

# structured trace
# is the trace being
# collected in the
# form of objects and
# exposed to the user
# in json or any other
# convenient form.
vm.structured {
    trace = false
    dir = vmtrace
    compressed = true
    initStorageLimit = 10000

    # enables storage disctionary db recording
    # see the org.ehereum.db.StorageDisctionary class for details
    storage.dictionary.enabled = false
}

# make changes to tracing options
# starting from certain block
# -1 don't make any tracing changes
trace.startblock = -1

# invoke vm program on
# message received,
# if the vm is not invoked
# the balance transfer
# occurs anyway  [true/false]
play.vm = true

# hello phrase will be included in
# the hello message of the peer
hello.phrase = Dev

# this property used
# mostly for a debug purpose
# so if you don't know exactly how
# to apply it leave to be [-1]
#
# ADVANCED: if we want to load a root hash
# for db not from the saved block chain (last block)
# but any manual hash this property will help.
# values [-1] - load from db
#        [hex hash 32 bytes] root hash
root.hash.start = null

# Key value data source values: [leveldb/redis/mapdb]
keyvalue.datasource = leveldb

# Redis cloud enabled flag.
# Allows using RedisConnection for creating cloud based data structures.
redis.enabled=false

record.blocks=false
blockchain.only=false

# Load the blocks
# from a rlp lines
# file and not for
# the net
blocks.loader=""


# the parameter speciphy when exactly
# to switch managing storage of the
# account on autonomous db
details.inmemory.storage.limit=1000

# cache for blockchain run
# the flush hapens depending
# on memory usage or blocks
# treshhold if both specipied
# memory will take precedence
cache {

    flush {

        # [0.7 = 70% memory to flush]
        memory = 0

        # [10000 flush each 10000 blocks]
        blocks = 1
    }
}

# eth sync process
sync {

    # block chain synchronization
    # can be: [true/false]
    enabled = true

    # maximum blocks hashes to ask.
    # sending GET_BLOCK_HASHES msg
    # we specify number of block we want
    # to get, recomendec value [1..1000]
    # Default: unlimited
    max.hashes.ask = 10000

    # maximum blocks to ask,
    # when downloading the chain
    # sequenteally sending GET_BLOCKS msg
    # we specify number of blocks we want
    # to get, recomendec value [1..120]
    max.blocks.ask = 100

    # minimal peers count
    # used in sync process
    # sync may use more peers
    # than this value
    # but always trying to get
    # at least this number from discovery
    peer.count = 10

    # whether to wait for blockchain sync to start mining and pegging.
    waitForSync = false

    # whether to do a System.exit() if something happens outside the "success flow"
    exitOnBlockConflict = true
}

# miner options
mine {

    # start mining blocks
    # when 'sync.enabled' is true the mining starts when the sync is complete
    # else the mining will start immediately taking the best block from database
    # (or genesis if no blocks exist yet)
    start = false

    # mining beneficiary
    coinbase = "50b8f981ce93fd5b81b844409169148428400bf3"

    # extra data included to the mined block
    # one of two properties should be specified
    extraData = "EthereumJ powered"
    #extraDataHex = "0102abcd"

    # transactions with the gas price lower than this will not be
    # included to mined blocks
    # decimal number in weis
    minGasPrice = 1  # 50 Gwei

    # minimal timeout between mined blocks
    minBlockTimeoutMsec = 0

    # number of CPU threads the miner will mine on
    # 0 disables CPU mining
    cpuMineThreads = 1

    # there two options for CPU mining 'light' and 'full'
    # 'light' requires only 16M of RAM but is much slower
    # 'full' requires 1G of RAM and possibly ~7min for the DataSet generation
    #   but is much faster during mining
    fullDataSet = false
    lightDataSet = true
}

miner {
    server.enabled = true
    client.enabled = false
}

simulateTxs {
    enabled = false
}

rpc {
    enabled = true
    port = 4444
}

federator {
    enabled = false
    secret = federator1
}
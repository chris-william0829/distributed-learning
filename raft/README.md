Client (gRPC)
    |
    V
Raft Leader (gRPC Server)
    |       \
    V        V
Raft Follower1  Raft Follower2
    |               |
    V               V
KV Store1        KV Store2

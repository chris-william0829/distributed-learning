// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: raftRpc.proto

#include "raftRpc.pb.h"
#include "raftRpc.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/impl/client_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/rpc_service_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/sync_stream.h>
namespace raftRpc {

static const char* RaftRpcService_method_names[] = {
  "/raftRpc.RaftRpcService/RequestVote",
  "/raftRpc.RaftRpcService/AppendEntries",
  "/raftRpc.RaftRpcService/InstallSnapshot",
};

std::unique_ptr< RaftRpcService::Stub> RaftRpcService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< RaftRpcService::Stub> stub(new RaftRpcService::Stub(channel, options));
  return stub;
}

RaftRpcService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_RequestVote_(RaftRpcService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_AppendEntries_(RaftRpcService_method_names[1], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_InstallSnapshot_(RaftRpcService_method_names[2], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status RaftRpcService::Stub::RequestVote(::grpc::ClientContext* context, const ::raftRpc::RequestVoteRequest& request, ::raftRpc::RequestVoteResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::raftRpc::RequestVoteRequest, ::raftRpc::RequestVoteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_RequestVote_, context, request, response);
}

void RaftRpcService::Stub::async::RequestVote(::grpc::ClientContext* context, const ::raftRpc::RequestVoteRequest* request, ::raftRpc::RequestVoteResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::raftRpc::RequestVoteRequest, ::raftRpc::RequestVoteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_RequestVote_, context, request, response, std::move(f));
}

void RaftRpcService::Stub::async::RequestVote(::grpc::ClientContext* context, const ::raftRpc::RequestVoteRequest* request, ::raftRpc::RequestVoteResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_RequestVote_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::RequestVoteResponse>* RaftRpcService::Stub::PrepareAsyncRequestVoteRaw(::grpc::ClientContext* context, const ::raftRpc::RequestVoteRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::raftRpc::RequestVoteResponse, ::raftRpc::RequestVoteRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_RequestVote_, context, request);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::RequestVoteResponse>* RaftRpcService::Stub::AsyncRequestVoteRaw(::grpc::ClientContext* context, const ::raftRpc::RequestVoteRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncRequestVoteRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status RaftRpcService::Stub::AppendEntries(::grpc::ClientContext* context, const ::raftRpc::AppendEntriesRequest& request, ::raftRpc::AppendEntriesResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::raftRpc::AppendEntriesRequest, ::raftRpc::AppendEntriesResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_AppendEntries_, context, request, response);
}

void RaftRpcService::Stub::async::AppendEntries(::grpc::ClientContext* context, const ::raftRpc::AppendEntriesRequest* request, ::raftRpc::AppendEntriesResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::raftRpc::AppendEntriesRequest, ::raftRpc::AppendEntriesResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_AppendEntries_, context, request, response, std::move(f));
}

void RaftRpcService::Stub::async::AppendEntries(::grpc::ClientContext* context, const ::raftRpc::AppendEntriesRequest* request, ::raftRpc::AppendEntriesResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_AppendEntries_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::AppendEntriesResponse>* RaftRpcService::Stub::PrepareAsyncAppendEntriesRaw(::grpc::ClientContext* context, const ::raftRpc::AppendEntriesRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::raftRpc::AppendEntriesResponse, ::raftRpc::AppendEntriesRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_AppendEntries_, context, request);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::AppendEntriesResponse>* RaftRpcService::Stub::AsyncAppendEntriesRaw(::grpc::ClientContext* context, const ::raftRpc::AppendEntriesRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncAppendEntriesRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status RaftRpcService::Stub::InstallSnapshot(::grpc::ClientContext* context, const ::raftRpc::InstallSnapshotRequest& request, ::raftRpc::InstallSnapshotResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::raftRpc::InstallSnapshotRequest, ::raftRpc::InstallSnapshotResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_InstallSnapshot_, context, request, response);
}

void RaftRpcService::Stub::async::InstallSnapshot(::grpc::ClientContext* context, const ::raftRpc::InstallSnapshotRequest* request, ::raftRpc::InstallSnapshotResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::raftRpc::InstallSnapshotRequest, ::raftRpc::InstallSnapshotResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InstallSnapshot_, context, request, response, std::move(f));
}

void RaftRpcService::Stub::async::InstallSnapshot(::grpc::ClientContext* context, const ::raftRpc::InstallSnapshotRequest* request, ::raftRpc::InstallSnapshotResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InstallSnapshot_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::InstallSnapshotResponse>* RaftRpcService::Stub::PrepareAsyncInstallSnapshotRaw(::grpc::ClientContext* context, const ::raftRpc::InstallSnapshotRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::raftRpc::InstallSnapshotResponse, ::raftRpc::InstallSnapshotRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_InstallSnapshot_, context, request);
}

::grpc::ClientAsyncResponseReader< ::raftRpc::InstallSnapshotResponse>* RaftRpcService::Stub::AsyncInstallSnapshotRaw(::grpc::ClientContext* context, const ::raftRpc::InstallSnapshotRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncInstallSnapshotRaw(context, request, cq);
  result->StartCall();
  return result;
}

RaftRpcService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RaftRpcService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< RaftRpcService::Service, ::raftRpc::RequestVoteRequest, ::raftRpc::RequestVoteResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](RaftRpcService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::raftRpc::RequestVoteRequest* req,
             ::raftRpc::RequestVoteResponse* resp) {
               return service->RequestVote(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RaftRpcService_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< RaftRpcService::Service, ::raftRpc::AppendEntriesRequest, ::raftRpc::AppendEntriesResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](RaftRpcService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::raftRpc::AppendEntriesRequest* req,
             ::raftRpc::AppendEntriesResponse* resp) {
               return service->AppendEntries(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RaftRpcService_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< RaftRpcService::Service, ::raftRpc::InstallSnapshotRequest, ::raftRpc::InstallSnapshotResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](RaftRpcService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::raftRpc::InstallSnapshotRequest* req,
             ::raftRpc::InstallSnapshotResponse* resp) {
               return service->InstallSnapshot(ctx, req, resp);
             }, this)));
}

RaftRpcService::Service::~Service() {
}

::grpc::Status RaftRpcService::Service::RequestVote(::grpc::ServerContext* context, const ::raftRpc::RequestVoteRequest* request, ::raftRpc::RequestVoteResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status RaftRpcService::Service::AppendEntries(::grpc::ServerContext* context, const ::raftRpc::AppendEntriesRequest* request, ::raftRpc::AppendEntriesResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status RaftRpcService::Service::InstallSnapshot(::grpc::ServerContext* context, const ::raftRpc::InstallSnapshotRequest* request, ::raftRpc::InstallSnapshotResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace raftRpc


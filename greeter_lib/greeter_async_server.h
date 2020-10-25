#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

using helloworld::PlateInRequest;
using helloworld::PlateInReply;

class ServerImpl final {
  public:
    ~ServerImpl() {
      server_->Shutdown();
      // Always shutdown the completion queue after the server.
      cq_->Shutdown();
    }

    // There is no shutdown handling in this code.
    void Run();

 private:
    // This can be run in multiple threads if needed.
    void HandleRpcs();

    std::unique_ptr<ServerCompletionQueue> cq_;
    Greeter::AsyncService service_;
    std::unique_ptr<Server> server_;
};

class CallDataBase
{
  public:
    virtual void Proceed() = 0;
    virtual ~CallDataBase() {}
};
// Class encompasing the state and logic needed to serve a request.
template<class req, class rep>
class CallData : public CallDataBase
{
  public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(Greeter::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // // Invoke the serving logic right away.
      // Proceed();
    }
    virtual void Proceed() = 0;
protected:
// The means of communication with the gRPC runtime for an asynchronous
    // server.
    Greeter::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    req request_;
    // What we send back to the client.
    rep reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<rep> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
};

class HelloCall : public CallData<HelloRequest, HelloReply>
{
  public:
    void Proceed();
    HelloCall(Greeter::AsyncService* service, ServerCompletionQueue* cq)
        : CallData(service, cq) {
          Proceed();
        }
};

class PlateInCall : public CallData<PlateInRequest, PlateInReply>
{
  public:
    void Proceed();
    PlateInCall(Greeter::AsyncService* service, ServerCompletionQueue* cq)
        : CallData(service, cq){
          Proceed();
        }
};
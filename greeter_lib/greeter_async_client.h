
#include <iostream>
#include <memory>
#include <string>
#include<atomic>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;
using helloworld::PlateInRequest;
using helloworld::PlateInReply;

class GreeterAsyncClient {
  public:
    explicit GreeterAsyncClient(std::shared_ptr<Channel> channel)
            : stub_(Greeter::NewStub(channel)) {}

    
    void SayHello(const std::string& user);
    void UploadPlateIn(const std::string& plate);
    // Loop while listening for completed responses.
    // Prints out the response from the server.
    void AsyncCompleteRpc();

  private:

    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<Greeter::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue cq_;
};
class AsyncClientCall
{
  public:
    virtual ~AsyncClientCall(){}
    virtual std::string message() = 0;
    ClientContext context;
    Status status;
};
template<class T>
class AsyncClientCallBase : public AsyncClientCall
{
public:
    T reply;
    std::unique_ptr<ClientAsyncResponseReader<T>> response_reader; 
};

class AsyncClientHelloCall : public AsyncClientCallBase<HelloReply>
{
  public:
    virtual std::string message();
};
class  AsyncClientPlateInCall : public AsyncClientCallBase<PlateInReply>
{
public:
    virtual std::string message();
};
/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "greeter_async_server.h"

void ServerImpl::Run()
{
  std::string server_address("0.0.0.0:50051");

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service_" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *asynchronous* service.
  builder.RegisterService(&service_);
  // Get hold of the completion queue used for the asynchronous communication
  // with the gRPC runtime.
  cq_ = builder.AddCompletionQueue();
  // Finally assemble the server.
  server_ = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;

  // Proceed to the server's main loop.
  HandleRpcs();
}

// This can be run in multiple threads if needed.
void ServerImpl::HandleRpcs()
{
  // Spawn a new CallData instance to serve new clients.
  new HelloCall(&service_, cq_.get());
  new PlateInCall(&service_, cq_.get());
  void* tag;  // uniquely identifies a request.
  bool ok;
  while (true) {
    // Block waiting to read the next event from the completion queue. The
    // event is uniquely identified by its tag, which in this case is the
    // memory address of a CallData instance.
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or cq_ is shutting down.
    GPR_ASSERT(cq_->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallDataBase*>(tag)->Proceed();
  }
}

void HelloCall::Proceed() 
{
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    // As part of the initial CREATE state, we *request* that the system
    // start processing SayHello requests. In this request, "this" acts are
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_,
                              this);
  } else if (status_ == PROCESS) {
    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state.
    // new HelloCall(service_, cq_);
    // new CallData(service_, cq_);
    new HelloCall(service_, cq_);

    // The actual processing.
    std::string prefix("Hello ");
    reply_.set_message(prefix + request_.name());

    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    // Once in the FINISH state, deallocate ourselves (CallData).
    delete this;
  }
}

void PlateInCall::Proceed()
{
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    // As part of the initial CREATE state, we *request* that the system
    // start processing SayHello requests. In this request, "this" acts are
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    service_->RequestUploadPlateIn(&ctx_, &request_, &responder_, cq_, cq_,
                              this);
  } else if (status_ == PROCESS) {
    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state.
    new PlateInCall(service_, cq_);

    // The actual processing.
    // std::string prefix("Hello ");
    // reply_.set_message(prefix + request_.name());
    reply_.set_plate(request_.plate());
    reply_.set_plate_type(helloworld::PlateType::TEMPORARY);

    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    // Once in the FINISH state, deallocate ourselves (CallData).
    delete this;
  }
}
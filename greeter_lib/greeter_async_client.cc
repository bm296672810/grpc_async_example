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
#include "greeter_async_client.h"

// Assembles the client's payload and sends it to the server.
void GreeterAsyncClient::SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Call object to store rpc data
    AsyncClientHelloCall* call = new AsyncClientHelloCall;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncSayHello(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call object.
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    std::atomic_int i(0);
}
void GreeterAsyncClient::UploadPlateIn(const std::string& plate)
{
    PlateInRequest request;
    request.set_plate(plate);

    AsyncClientPlateInCall* call = new AsyncClientPlateInCall;
    call->response_reader = stub_->PrepareAsyncUploadPlateIn(&call->context, request, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}
// Loop while listening for completed responses.
// Prints out the response from the server.
void GreeterAsyncClient::AsyncCompleteRpc() {
    void* got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
        // The tag in this example is the memory location of the call object
        AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
        GPR_ASSERT(ok);

        if (call->status.ok())
            std::cout << "Greeter received: " << call->message() << std::endl;
        else
            std::cout << "RPC failed" << std::endl;

        // Once we're complete, deallocate the call object.
        delete call;
    }
    
}

std::string AsyncClientHelloCall::message()
{
    return reply.message();
}

std::string AsyncClientPlateInCall::message()
{
    return reply.plate() + ":" + std::to_string(reply.plate_type());
}
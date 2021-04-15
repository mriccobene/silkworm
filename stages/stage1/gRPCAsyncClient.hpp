//
// Created by miche on 08/04/2021.
//

#ifndef SILKWORM_GRPCASYNCCLIENT_HPP
#define SILKWORM_GRPCASYNCCLIENT_HPP

#include <iostream>
#include <chrono>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <silkworm/db/access_layer.hpp>
#include <silkworm/db/stages.hpp>
#include <silkworm/db/util.hpp>
#include <silkworm/db/tables.hpp>

#include <silkworm/chain/config.hpp>

#include <interfaces/sentry.grpc.pb.h>

#include "Types.hpp"

namespace silkworm::rpc {

// AsyncCall ----------------------------------------------------------------------------------------------------------
// A generic async RPC call with preparation, executing and reply handling
template <class STUB>
class AsyncCall {
    // See concrete implementation for a public constructor
    // Use AsyncClient to send this call to a remote server

  protected:
    // Start the remote proc call sending request to the server
    virtual void start(typename STUB::Stub* stub, grpc::CompletionQueue* cq) = 0;

    // Will be called on response arrival from the server
    virtual void complete(bool ok) = 0;


    // See concrete implementations for a public constructor
    AsyncCall() { tag_ = static_cast<void*>(this); }

    // Tag used by GRPCAsyncClient to identify concrete instances in the receiving loop
    void* tag_;

    // Call-context, it could be used to convey extra information to the server and/or tweak certain RPC behaviors
    grpc::ClientContext context_;

    // Status of the RPC upon completion
    grpc::Status status_;
};

// AsyncClient ----------------------------------------------------------------------------------------------------
template <class STUB>
class AsyncClient {
  public:
    using stub_t = STUB;
    using call_t = AsyncCall<STUB>;

    explicit AsyncClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(stub_t::NewStub(channel)) {}

    // send an remote call asynchronously
    void send(call_t* call) {
        call->start(stub_, completionQueue_);
    }

    // wait for a response and complete it; put this call in a infinite loop:
    //    while(client.receive_one())
    //       ; // does nothing
    bool receive_one() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue
        bool got_event = completionQueue_.Next(&got_tag, &ok);  // todo: use AsyncNext to support a timeout
        if (!got_event) // the queue is fully drained and shut down
            return false;

        // The tag in this example is the memory location of the call object
        call_t* call = static_cast<call_t*>(got_tag);

        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
        // GPR_ASSERT(ok);  // works only for unary call

        // Delegate status & reply handling to the call
        call->complete(ok);

        // Once we're complete, deallocate the call object
        // if (...)
        //    delete call;  // todo: here or in call->complete()?
        return true;
    }

  protected:

    /*
    // Loop while listening for completed responses.
    static void receiving_loop() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (completionQueue_.Next(&got_tag, &ok)) {  // todo: use AsyncNext to test exiting_
            // The tag in this example is the memory location of the call object
            AsyncCall* call = static_cast<AsyncCall*>(got_tag);

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            // GPR_ASSERT(ok);  // works only for unary call

            // Delegate status & reply handling to the call
            call->complete(ok);

            // Once we're complete, deallocate the call object
            // if (...)
            //    delete call;  // todo: here or in call->complete()?
        }
    }
    */

    std::unique_ptr<typename STUB::Stub> stub_; // the stub class generated from grpc proto
    grpc::CompletionQueue completionQueue_;  // receives async-call completion events from the gRPC runtime
};

// AsyncClientCall ----------------------------------------------------------------------------------------------------
template <class STUB, class REQUEST, class REPLY>
class AsyncUnaryCall: public AsyncCall<STUB> {
  public:
    using call_t = AsyncCall<STUB>;
    using request_t = REQUEST;
    using reply_t = REPLY;
    using response_reader_t = std::unique_ptr<grpc::ClientAsyncResponseReader<reply_t>>;
    using prepare_call_t = response_reader_t (STUB::Stub::*)(grpc::ClientContext* context, const request_t& request, grpc::CompletionQueue* cq);
    using callback_t = std::function<void(AsyncUnaryCall&)>;
    using call_t::context_, call_t::status_, call_t::tag_;

    AsyncUnaryCall(prepare_call_t pc, request_t request) : call_t{}, prepare_call_{pc}, request_t{std::move(request)} {
    }

    void on_complete(callback_t f) {callback_ = f;}

  protected:

    void start(typename STUB::Stub* stub, grpc::CompletionQueue* completionQueue) override {
        response_reader_ = (stub->*prepare_call_)(&context_, request_, completionQueue);  // creates an RPC object

        response_reader_->StartCall();  // initiates the RPC call

        response_reader_->Finish(&reply_, &status_, tag_);  // communicate replay & status slots and tag
    }

    void complete(bool ok) override {
        // only for test
#if !defined(NDEBUG)
        if (status_.ok())
            std::cout << "RPC ok";
        else
            std::cerr << "RPC failed, error: " << status_.error_message() << ", code: " << status_.error_code()
                      << ", details: " << status_.error_details() << std::endl;
#endif

        // use status & reply
        if (callback_)
            callback_(*this);
    }

    prepare_call_t prepare_call_; // Pointer to the prepare call method of the Stub

    response_reader_t response_reader_; // return type of the prepare call method of the Stub

    request_t request_; // Container for the request we send to the server

    reply_t reply_; // Container for the data we expect from the server.

    callback_t callback_; // Callback that will be called on completion (i.e. response arrival)
};

// AsyncClientCall --------------------------------------------------------------------------------------------------
// A generic async RPC call with preparation, executing and reply handling
template <class STUB, class REQUEST, class REPLY>
class AsyncOutStreamingCall: public AsyncCall<STUB> {
  public:
    using call_t = AsyncCall<STUB>;
    using request_t = REQUEST;
    using reply_t = REPLY;
    using response_reader_t = std::unique_ptr<::grpc::ClientAsyncReader<reply_t>>;
    using prepare_call_t = response_reader_t (STUB::Stub::*)(grpc::ClientContext* context, const request_t& request, grpc::CompletionQueue* cq);
    using callback_t = std::function<void(AsyncOutStreamingCall&)>;
    using call_t::context_, call_t::status_, call_t::tag_;

    AsyncOutStreamingCall(prepare_call_t pc, const request_t& request) : call_t{}, prepare_call_{pc}, request_t{std::move(request)} {
    }

    ~AsyncOutStreamingCall() {
        response_reader_->Finish(&status_, tag_);  // communicate replay & status slots and tag

        // only for test
#if !defined(NDEBUG)
        if (status_.ok())
            std::cout << "RPC ok";
        else
            std::cerr << "RPC failed, error: " << status_.error_message() << ", code: " << status_.error_code()
                      << ", details: " << status_.error_details() << std::endl;
#endif
    }

    void on_complete(callback_t f) {callback_ = f;}

  protected:
    void start(typename STUB::Stub* stub, grpc::CompletionQueue& cq) override {
        response_reader_ = stub->PrepareAsyncReceiveMessages(&context_, request_, &cq);  // creates an RPC object

        response_reader_->StartCall(tag_);  // initiates the RPC call

        response_reader_->Read(&reply_, tag_); // per quelle stream in output vanno chiamate le Read ogni volta per ottenere una risposta
    }

    void complete(bool ok) override {
        if (!ok) // todo: check if it is correct!
            return; // todo: return or raise exception?

        // use status & reply
        // ...
        if (callback_)
            callback_(*this);

        // only for test
#if !defined(NDEBUG)
        if (status_.ok())
            std::cout << "RPC ok";
        else
            std::cerr << "RPC failed, error: " << status_.error_message() << ", code: " << status_.error_code()
                      << ", details: " << status_.error_details() << std::endl;
#endif

        // todo: erase reply_ ?
        reply_ = {};

        // request next message
        response_reader_->Read(&reply_, tag_);
    }

    prepare_call_t prepare_call_; // Pointer to the prepare call method of the Stub

    response_reader_t response_reader_; // return type of the prepare call method of the Stub

    request_t request_; // Container for the request we send to the server

    reply_t reply_; // Container for the data we expect from the server

    callback_t callback_; // Callback that will be called on completion (i.e. response arrival)
};

} // namespace end

#endif  // SILKWORM_GRPCASYNCCLIENT_HPP

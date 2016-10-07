#include <QCoreApplication>
#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <grpc++/grpc++.h>

#include "main.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using todolist::Void;
using todolist::Task;
using todolist::Event;
using todolist::TodoList;

class CallData {
 public:
  CallData(Event::AsyncService* service, ServerCompletionQueue* cq)
      : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
    // Invoke the serving logic right away.
    Proceed();
  }

  void Proceed() {
    if (status_ == CREATE) {
      status_ = PROCESS;

      /*
       * *this* must be a unique identifier
       */
      service_->RequestOnTask(&ctx_, &request_, &responder_, cq_, cq_,this);
    } else if (status_ == PROCESS) {

      new CallData(service_, cq_);

      // The actual processing.
      //reply_.set_name("Go to the gym");

      //status_ = FINISH;
      //responder_.Finish(reply_, Status::OK, this);
    } else {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }
  }

 private:

  Event::AsyncService* service_;
  ServerCompletionQueue* cq_;
  ServerContext ctx_;

  Void request_;
  Task reply_;
  ServerAsyncResponseWriter<Task> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;  // The current serving state.
};

// Logic and data behind the server's behavior.
class TodoListServiceImpl final : public TodoList::Service {

  Status AddTask(ServerContext* context, const Task* request,
                    Void* reply) override {

    std::cout << "A Task is being add" << std::endl;

    //Call async service here now
    reply_.set_name("Go to the gym");
    status_ = FINISH;
    /*
     * Problem *this* must be the tag that we use to register the request...
     */
    responder_.Finish(reply_, Status::OK, this);

    return Status::OK;
  }

  public:
    TodoListServiceImpl(Event::AsyncService* service, ServerCompletionQueue* cq)
      : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) { }

  private:
    Event::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;

    Void request_;
    Task reply_;
    ServerAsyncResponseWriter<Task> responder_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
};

class ServerImpl final {

 public:

  ~ServerImpl() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  void Run() {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();

    TodoListServiceImpl service2_(&service_,cq_.get());
    builder.RegisterService(&service2_);

    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
    //server_->Wait();
  }

 private:

  // This can be run in multiple threads if needed.
  void HandleRpcs() {
    new CallData(&service_, cq_.get());
    void* tag;
    bool ok;
    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      std::cout << "lol" << std::endl;
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  Event::AsyncService service_;
  //TodoListServiceImpl service2_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ServerImpl server;
    server.Run();

    return a.exec();
}

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
      /*
       * *this* must be a unique identifier
       */
      std::cout << "this " << this << std::endl;
      service_->RequestOnTask(&ctx_, &request_, &responder_, cq_, cq_,this);

      status_ = PROCESS;
    } else if (status_ == PROCESS) {

      new CallData(service_, cq_);

    } else {
      GPR_ASSERT(status_ == FINISH);
      std::cout << "Destroy" << std::endl;
      delete this;
    }
  }

  void Announce(Task task) {
    if(status_ != PROCESS){
      return;
    }

    status_ = FINISH;

    reply_.set_name("Go to the gym");
    responder_.Finish(task, Status::OK, this);

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


    reply_.set_name("Go to the gym");
    std::cout << "AddTask : " << tag << std::endl;

    if(tag)
      static_cast<CallData*>(tag)->Announce(reply_);

    return Status::OK;
  }

  public:
    TodoListServiceImpl()
      : tag(nullptr){

    }

    void setTag(void* t) {
        std::cout << "setTag : " << t << std::endl;
        tag = t;
    }

  private:
    ServerContext ctx_;

    Void request_;
    Task reply_;

    // Keep tags
    void* tag;
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

    cq_ = builder.AddCompletionQueue();
    builder.RegisterService(&service_);//async service

    TodoListServiceImpl service2_;
    builder.RegisterService(&service2_);//sync service

    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs(&service2_);
    //server_->Wait();
  }

 private:

  // This can be run in multiple threads if needed.
  void HandleRpcs(TodoListServiceImpl *service2_) {
    new CallData(&service_, cq_.get());
    void* tag;
    bool ok;
    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      std::cout << "Lol" << std::endl;
      service2_->setTag(tag);
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

    ServerImpl server;
    server.Run();

    return 0;
}

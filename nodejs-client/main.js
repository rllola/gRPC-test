var express = require('express')
var app = express()

var PROTO_PATH = __dirname + '/../hello/main.proto'

var grpc = require('grpc')

var stub = grpc.load(PROTO_PATH).todolist

var clientEvent = new stub.Event('localhost:50051', grpc.credentials.createInsecure());
var clientTodoList = new stub.TodoList('localhost:50051', grpc.credentials.createInsecure());

var onTask = function(callback) {
  clientEvent.onTask({}, callback)
}

var addTask = function(task, callback) {
  clientTodoList.addTask(task, callback);
}

app.listen(3000, function () {
  console.log('Example app listening on port 3000!')

  onTask(function(err, task) {
    if (err) {
      console.log('Something wrong')
    } else {
      console.log('Task :', task)
    }
  })

  var t = {'name': 'Go to thailand'}

  addTask(t, function(err, data) {
    if (err) {
      console.log('Draaaaa')
    } else {
      console.log('data recieved: ', data)
    }
  })

})

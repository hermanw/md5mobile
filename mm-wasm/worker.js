importScripts("wasm.js");

var data;
onmessage = function (e) {
    data = e.data;
    if (Module.decode == undefined) {
        // wait for wasm loaded
        setTimeout("decode(data)", 500)
    } else {
        decode(data);
    }
}

function decode(data) {
    var thread_num = data.thread_num;
    var threadid = data.threadid;
    buf = newString(Module, data.input);
    Module.decode(buf, thread_num, threadid);
    Module.dealloc_str(buf);
}

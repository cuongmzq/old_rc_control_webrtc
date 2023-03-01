document.addEventListener("DOMContentLoaded", function(){
    mainVideo = document.getElementById("video_player");
    buttonCall = document.getElementById("btn_call");
    buttonHangup = document.getElementById("btn_hangup");

    let signalObj = null;

    function call() {
        if (signalObj) return;

        let wsURL = "ws://" + location.host + "/webrtc";
        signalObj = new signal(wsURL, onStream, onError, onClose, onMessage);

        function onStream(src) {
            mainVideo.srcObject = src;

            //video.srcObject = src || event.streams[0];
        }

        function onError(err) {
            console.log(err);
        }

        function onClose() {
            console.log("close socket");
            signalObj = null;
        }

        function onMessage(msg) {
            console.log("message: " + msg);
        }
    }

    function hangup() {
        if (signalObj) {
            signalObj.hangup();
            signalObj = null;
        }
    }

    buttonCall.addEventListener('click', function (e) {
        call();
    }, false);

    buttonHangup.addEventListener('click', function (e) {
        hangup();
    }, false);
});

gst-launch-1.0 rtpbin name=rtpbin dxgiscreencapsrc  !  queue ! decodebin ! videoconvert ! video/x-raw,format=I420,framerate=30/1 ! nvh264enc preset=1 bitrate=5000 rc-mode=2 gop-size=10  !h264parse config-interval=-1 ! rtph264pay pt=107 ssrc=2222 !rtprtxqueue max-size-time=2000 max-size-packets=0 ! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 ! udpsink host=124.71.159.87 port=44675 rtpbin.send_rtcp_src_0 ! udpsink host=124.71.159.87 port=40006 sync=false async=false

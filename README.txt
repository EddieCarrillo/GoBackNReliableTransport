# GoBackNReliableTransport

In general we will use a seq_num modoulo 2**32

#Sender Side Logic:

_____Sender init_____
Initiailize nextSeqNum = 1
Initiailize base = 1

_____Sender_Receives_Data_From_Upper_Layer____
If upper layer wants to send data:
    If there sender window is not full:
        Create a new transport layer packet with the upper layer message.
        Send the newly created packet to the lower (network layer)
        If we are creating a new windows:
            Start the packet loss timer
        Update the nextSeqNum since we can accept an N length window.
    otherwise if the windows is full:
        then ignore the packet (not realistic in a real implementation)

___Sender_A_Timeout_Event_Occurs_____
If there is a timeout event:
    Restart the timer
    Resend all of the packets in the window.

___Sender_Receives_Response_From_Receiver___
If the sender receives data from the lower layer AND the data is not corrupt
    Update the window base (Slide it over to the right as appropriate)
    If the update causes the windows to close:
        then we should stop the timer since are not waiting for any ACKs
    Otherwise:
        We should restart the timer since we have new base and are still waiting for ACKs


______Receiver_Gets_Data_From_Lower_Layer_______
If receiver gets packet from lower layer AND the packet is not corrupt AND it has the correct nextseqnum value:
    Then we have a valid packet so let's extract the data
    Send the data to the upper layer
    Send an ACK with the nextseqnum as the value
    Update the nextseqnum value
Otherwise:
    resend ACK for last successfully received packet.




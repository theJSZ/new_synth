#include "midiReader.h"

static std::queue<std::pair<int, std::pair<int,int>>> messagebuffer;


static void midiCallback(double deltatime, std::vector<unsigned char>* bytes, void* userdata) {
    // Called when a message is received
    if (bytes->size() < 2) return;
    if (bytes->at(0) > 239) return;    // Message type 240 and up are system messages
                                       // we don't want them

                                       // By the MIDI standard, most messages are three bytes.
                                       // Byte 1 denotes message type and MIDI channel
                                       // so we get them by bitmasking
    long type = bytes->at(0) & 0xF0;   // 0xF0 = 0b11110000
    int channel = bytes->at(0) & 0x0F; // 0x0F = 0b00001111
    if (type != __SK_NoteOn_ && type != __SK_NoteOff_) return; // discard messages other than noteOn and noteOff

    channel += 1;

    // Bytes 2 and 3 are data bytes.
    long databyte1 = bytes->at(1);  // databyte 1 is note number
    long databyte2 = bytes->at(2);  // databyte 2 is velocity
    // long databyte2 = 0;

    // if ((type != 0xC0) && (type != 0xD0)) { // 0xC0 is Program Change, 0xD0 is aftertouch
    //                                         // they only have one data byte
    //     if (bytes->size() < 3) return;
    //     databyte2 = bytes->at(2);
    // }

    std::string typeName;
    switch(type){
        case __SK_NoteOn_:
            typeName = "Note on\t";
        case __SK_NoteOff_:
            typeName = "Note off\t";

        case __SK_ControlChange_:
            typeName = "ControlChange\t";
            break;
        default:
            typeName = "Unknown\t";
    }

    if (type == 0xB0) { // 0xB0 = 0b10110000 = Control Change
        if (typeName == "ControlChange\t") { // should always be true at this point?
            messagebuffer.push({channel, {databyte1, databyte2}});
        }
    } else {
        std::cout << "unknown" << std::endl;
    }

}

MidiReader::MidiReader(voiceAllocator* allocator) {
    std::cout << "Creating MIDI input" << std::endl;

    try {
      midi_in = new RtMidiIn();
    }
    catch (RtMidiError& e) {
        e.printMessage();
    }

    midi_in->setCallback(&midiCallback);
    midi_in->ignoreTypes(true, true, true); // ignore SysEx, timing and active sense

    std::cout << "Scanning available MIDI devices" << std::endl;
    unsigned int nPorts = midi_in->getPortCount();
    if (nPorts == 0) {
        std::cout << "No MIDI devices available :(" << std::endl;
        delete midi_in;
        // what else?
    }
    else {
        while(true) {
            std::cout << "Available MIDI devices:" << std::endl;
            for (unsigned int i = 0; i < nPorts; ++i) {
                std::cout << i << ": " << midi_in->getPortName(i) << std::endl;
            }
            std::cout << "Use which device??" << std::endl;
            if (std::cin >> port) break;
        }
    }

    std::cout << "Opening device \"" << midi_in->getPortName(port) << "\"" << std::endl;
    try {
        midi_in->openPort(port);
    }
    catch (RtMidiError& e) {
        e.printMessage();
    }
}

MidiReader::~MidiReader() {
    delete midi_in;
}

void MidiReader::pollMidiEvents() {

}

float MidiReader::midiNoteToHz(int midiNote) {
    /* fm =  2^((m−69)/12) * (440 Hz)
        where m is the MIDI note number
    */
   return pow(2, ((float) midiNote-69) / 12.0f) * 440;
}
#ifndef EightSegmentDisplay_H
#define EightSegmentDisplay_H


class EightSegmentDisplay{
private:

byte segments[8];
byte mappedSegments[8];
byte mappedCharacter;
byte displayCharacters[];


public:
EightSegmentDisplay(byte bitsMSBFirst[]){
for (int i = 0;i<8;i++){
  segments[i]= bitsMSBFirst[i];
}
}
byte sort(byte input){
for (int i = 0;i<8;i++){
  mappedSegments[segments[i]] = bitRead(input,7-i);
}
for (int i = 0;i<8;i++){
  bitWrite(mappedCharacter,7-i,mappedSegments[i]);
}
return mappedCharacter;
}
};


#endif
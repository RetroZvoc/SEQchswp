/*

    SEQchswp v1.0: SEQ Channel Swapper
    By RetroZvoc

    License: MIT

    Copyright 2019 RetroZvoc

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char** argv)
{
    // Nice title

    puts("SEQchswp v1.0: SEQ Channel Swapper.\nBy RetroZvoc\n");
    do
    {
        // Check if there's a filepath in the argument

        if(argc!=2)
        {
            printf("To convert a SEQ file, simply drag it onto SEQchswp.exe or append its path to SEQchswp.exe in the command line.\n");
            break;
        }

        // Load the file data into an array

        uint8_t *data;
        unsigned int len;
        FILE *f = fopen(argv[1], "rb");
        if (!f)
        {
            printf("Cannot open file at path %s\n",argv[1]);
            break;
        }
        else
        {
            printf("File %s opened! :D",argv[1]);
            fseek(f, 0, SEEK_END);
            len = ftell(f);
            if(len<15)
            {
                printf("SEQ data is too short!\n");
                fclose(f);
                break;
            }
            printf("SEQ data length OK. :) %d byte%c.\n",len,(len!=1)?'s':'\0');
            fseek(f, 0, SEEK_SET);
            data = (uint8_t*)malloc(len);
            if(!data)
            {
                printf("Malloc failed! Not enough RAM!\n");
                fclose(f);
                break;
            }
            else
            {
                printf("Reading SEQ data...\n");
                if(fread(data, sizeof(uint8_t), len, f)!=len)
                {
                    printf("Reading error! Closing file...\n");
                    if(fclose(f)==EOF)
                    {
                        printf("Closing error!\n");
                    }
                    break;
                }
                printf("SEQ data loaded. :D\n");
            }
            printf("Closing file...\n");
            if(fclose(f)==EOF)
            {
                printf("Closing error, but moving on. :o\n");
            }
            else
            {
                printf("Closed well! ^^\n");
            }
        }

        // Verify data on load

        uint32_t magicgot=data[3]|data[2]<<8|data[1]<<16|data[0]<<24;
        uint32_t magicwant=0x70514553;
        if(magicgot!=magicwant)
        {
            printf("SEQ magic number is invalid! It's 0x%X, but it should have been 0x%X! >:(\n",magicgot,magicwant);
            break;
        }
        printf("SEQ magic number OK! ^^\n");
        if(data[4]!=0)
        {
            printf("SEQ version number is wrong! It's %d, but it should be %d! >:C\n",data[4],0);
            break;
        }
        printf("SEQ version number OK! ^^\n");

        // Create a replacement table

        printf("Now we can get down to business... UwU\n\n");
        uint8_t replacements[16];
        for(uint8_t i=0;i<16;i++)
        {
            replacements[i]=i;
        }

        // Warn for backups

        printf("WARNING!: Always backup your SEQs! I cannot be responsible for possible dataloss!\n");

        // Ask for replacements

        bool done=false;
        do
        {
            printf("Here's your channel configuration: \n");
            for(uint8_t i=0;i<16;i++)
            {
                printf("%2d=%2d\t",i+1,replacements[i]+1);
            }
            putchar('\n');
            printf("What channels would you like to replace?\nType 0 and 0 to confirm.\n");
            int oldchnl;
            int newchnl;
            printf("Old=");
            scanf("%d",&oldchnl);
            printf("New=");
            scanf("%d",&newchnl);
            if(oldchnl*newchnl==0)
            {
                done=true;
            }
            else if(oldchnl>=1&&oldchnl<=16&&newchnl>=1&&newchnl<=16)
            {
                replacements[oldchnl-1]=newchnl-1;
            }
            else
            {
                printf("Channel numbers must be between 1 and 16. To stop inputing, type 0 and 0.\n");
            }
        }while(!done);

        // Start processing

        printf("Now processing your SEQ file...\n");
        uint8_t seeker=0;
        bool wellformed=false;
        // This seeker functions like this:
        // If set to 0, walker will read one byte and see what MIDI status code it represents
        // If greater than 0, it will skip that many bytes
        // But! If set to 255 (0xFF), it means it's looking for the meta event
        for(unsigned int walker=15;walker<len;walker++)
        {
            if(seeker>0) // We need to seek as many bytes as seeker says
            {
                if(seeker==0xFF) // Except if this is time to look up the meta event
                {
                    switch(*(data+walker))
                    {
                        case 0x51: // This is the tempo event
                            seeker=4; // It has 4 bytes which need to be skipped
                            break;
                        case 0x2F: // This is the end of the track
                            if((walker+2==len)&&(*(data+walker+1)=='\0')) // Check if there's a zero-byte at the end
                            {
                                wellformed=true; // The track is well-formed
                            }
                            else
                            {
                                printf("Track ended too early... It's not well-formed...\n");
                                walker=len-1; // The track is not well-formed because it ended too early
                            }
                            break;
                        default:
                            printf("SEQ track not well-formed. Meta-event byte at 0x%X is 0x%X which is unknown.\n",walker,*(data+walker));
                            walker=len-1; // Stop the walker because the track is not well-formed.
                            break;
                    }
                }
                else
                {
                    seeker--;
                    continue;
                }
            }
            uint8_t mybyte=(*(data+walker));
            if(mybyte==0xFF) // FF event
            {
                seeker=0xFF; // Next byte is a meta event byte!
            }
            else if(mybyte&0x80) // No deltatime?
            {
                switch(mybyte&0xF0)
                {
                case 0xC0: // 0xCX and 0xDX MIDI Statuses have 1 byte after them
                case 0xD0:
                    seeker=1;
                    break;
                default: // Others have 2 bytes after them
                    seeker=2;
                    break;
                }
                *(data+walker)=(mybyte&0xF0)|(replacements[mybyte&0x0F]);
            }
            else // Yes deltatime?
            {
                seeker=0;
            }
        }

        // Ask to save if not well-formed

        if(!wellformed)
        {
            char confirm;
            printf("The processed SEQ track is not well-formed. Save anyway? Type 'y' for yes and 'n' for no.\n");
            do
            {
                scanf("%c\n",&confirm);
                if(confirm=='y'||confirm=='n')
                {
                    break;
                }
                printf("You should type either 'y' or 'n'.\n");
            }
            while(1);
            if(confirm=='n')
            {
                printf("Nothing was wrecked. ^^\n");
                break;
            }
            else
            {
                printf("If you say so...\n");
            }
        }

        // Save data back to the file

        printf("Now exporting your file...\n");

        f = fopen(argv[1], "wb");
        if (!f)
        {
            printf("Cannot open file at path %s\n",argv[1]);
            break;
        }
        else
        {
            printf("File %s opened! :D Now gonna save into it! OwO\n",argv[1]);
            unsigned int outcome=fwrite(data, sizeof(uint8_t), len, f);
            if(outcome!=len)
            {
                printf("Oh no... OoO' Failed to write it...\n");
                break;
            }
            else
            {
                printf("YES!!! X3 We did it!\n");
            }
            printf("Closing your file...\n");
            if(fclose(f)==EOF)
            {
                printf("Closing error, but I hope it's okay. O.O'\n");
            }
            else
            {
                printf("Closed well! ^^\n");
            }
        }

        // Bye bye

        printf("Thank you for using SEQchswp! ^w^\n");
    }while(0);
    system("PAUSE");
    return 0;
}


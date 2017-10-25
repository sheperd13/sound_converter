#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>

#define FOUR_BYTES 1
#define TWO_BYTES 0

#define NUM_CHANNELS_OFFSET 22
#define SAMPLE_RATE_OFFSET 0
#define BITS_PER_SAMPLE_OFFSET 6
#define DATA_SIZE_OFFSET 4

unsigned char garbage_buffer[1024];
unsigned char buffer4[4];
unsigned char buffer2[2];

uint32_t little_to_big_endian(unsigned char bytes[], unsigned char four_bytes){
    uint32_t value;
    if(four_bytes == FOUR_BYTES){
        value = bytes[0] |
            (bytes[1] << 8) |
            (bytes[2] << 16) |
            (bytes[3] << 24);
    }else{
        value = bytes[0] |
            (bytes[1] << 8);
    }
    return value;
}

void little_to_big_endian_array(uint8_t little_endian_array[], uint8_t big_endian_array[], uint32_t array_size){
    uint32_t i;
    uint32_t j = array_size - 1;
    for(i = 0; i < array_size; i++){
        big_endian_array[i] = little_endian_array[j];
        j--;
    }
}

uint32_t get_number_of_samples(uint32_t data_size, uint32_t num_channels, uint32_t bits_per_sample){
    uint32_t samples = data_size/(num_channels * (bits_per_sample/8));
    return samples;
}

uint8_t get_size_of_number(char* num_samples, uint8_t buffer_size){
    uint8_t value = 0;
    for(uint8_t i = 0; i < buffer_size; i++){
        if(isdigit(num_samples[i])){
            value = i;
        }
    }
    return value;
}

int main(int argc, char** argv){
    char* wave_file = argv[1];
    char* sound_name = argv[2];
    printf("Size of sound_name: %lu\r\n", strlen(sound_name));

    int i = 0;

    uint32_t number_of_channels;
    uint32_t sample_rate;
    uint32_t data_size;
    uint32_t bits_per_sample;
    uint32_t number_of_samples;

    FILE * fp;
    fp = fopen(wave_file, "rb");

    //GET NUMBER OF CHANNELS
    fread(garbage_buffer, NUM_CHANNELS_OFFSET, 1, fp);
    fread(buffer2, sizeof(buffer2), 1, fp);
    number_of_channels = little_to_big_endian(buffer2, TWO_BYTES);
    printf("number_of_channels: %d\r\n", number_of_channels);

    //GET SAMPLE RATE
    fread(garbage_buffer, SAMPLE_RATE_OFFSET, 1, fp);
    fread(buffer4, sizeof(buffer4), 1, fp);
    sample_rate = little_to_big_endian(buffer4, FOUR_BYTES);
    printf("sample_rate: %d\r\n", sample_rate);

    //GET BITS PER SAMPLE
    fread(garbage_buffer, BITS_PER_SAMPLE_OFFSET, 1, fp);
    fread(buffer2, sizeof(buffer2), 1, fp);
    bits_per_sample = little_to_big_endian(buffer2, TWO_BYTES);
    printf("bits_per_sample: %d\r\n", bits_per_sample);

    //GET SIZE OF DATA
    fread(garbage_buffer, DATA_SIZE_OFFSET, 1, fp);
    fread(buffer4, sizeof(buffer4), 1, fp);
    data_size = little_to_big_endian(buffer4, FOUR_BYTES);
    printf("data_size: %d\r\n", data_size);

    //GET NUMBER OF SAMPLES
    number_of_samples = get_number_of_samples(data_size, number_of_channels, bits_per_sample);
    printf("number_of_samples: %d\r\n", number_of_samples);

    //GET DATA
    uint8_t data_little_endian[data_size];
    uint8_t data_big_endian[data_size];
    fread(data_little_endian, data_size, 1, fp);
    //I am still not sure about the format of the array. Come back here possibly
    //little_to_big_endian_array(data_little_endian, data_big_endian, data_size);

    fclose(fp);

    //START WRITING TO FILE
    char file_name[strlen(sound_name + 2)];    //allocate array for file name
    strcpy(file_name, sound_name);  //copy the sound name
    strcat(file_name, ".c");        //add .c at the end
    printf("file_name: %s\r\n", file_name);
    fp = fopen(file_name, "w");     //open a write connection to the file

    //WRITE THE SOUND DATA LINE INTO THE C FILE
    char line_of_code[28+strlen(sound_name)+data_size*3];   //allocate the right amount of space
                                                            //for the line of code, string
    strcpy(line_of_code, "uint32_t ");
    strcat(line_of_code, sound_name);
    strcat(line_of_code, "_soundData[] = {");
    char temp[2];   //temp char[] to hold the hex values of current sample byte
    //copy each byte into temp then add it and a comma, to line of code
    for(i = 0; i < data_size - 1; i++){
        sprintf(temp, "%02x", data_little_endian[i]);
        strcat(line_of_code, temp);
        strcat(line_of_code, ",");
    }
    sprintf(temp, "%02x", data_little_endian[data_size -1]);    //last byte w/o comma
    strcat(line_of_code, temp);
    strcat(line_of_code, "};\n\n");
    fwrite(line_of_code, sizeof(line_of_code), 1, fp);  //write line to .c file

    printf("%s", line_of_code);

    //WRITE THE NUMBER OF SAMPLES LINE INTO THE C CODE
    char num_samples[5];    //temp array
    sprintf(num_samples, "%d;\n\n", number_of_samples);
    uint8_t size_of_num_samples = get_size_of_number(num_samples, 5);
    char line_of_code1[31+strlen(sound_name)+size_of_num_samples];  //allocate space
    strcpy(line_of_code1, "uint32_t ");
    strcat(line_of_code1, sound_name);
    strcat(line_of_code1, "_numberOfSamples = ");
    strcat(line_of_code1, num_samples);
    fwrite(line_of_code1, sizeof(line_of_code1) + 1, 1, fp);    //write to file
    printf("%s", line_of_code1);

    //WRITE THE SAMPLE RATE LINE INTO THE C CODE
    char sample_rate_string[5]; //temp array
    sprintf(sample_rate_string, "%d;\n\n", sample_rate);
    uint8_t sizeof_sample_rate_string = get_size_of_number(num_samples, 5);
    char line_of_code2[26+strlen(sound_name)+sizeof_sample_rate_string];    //allocate space
    strcpy(line_of_code2, "uint32_t ");
    strcat(line_of_code2, sound_name);
    strcat(line_of_code2, "_sampleRate = ");
    strcat(line_of_code2, sample_rate_string);
    fwrite(line_of_code2, sizeof(line_of_code2) + 1, 1, fp);    //write to file
    printf("%s", line_of_code2);

    fclose(fp);
    return 0;
}

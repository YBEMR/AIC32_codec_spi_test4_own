#include "audio.h"
#include <string.h>

extern void UARTa_SendString(char *msg);
extern void UARTa_SendStringAndNumber(char *msg1, int32 number, char *msg2);

void I2CA_Init()
{
   // Initialize I2C
   I2caRegs.I2CSAR = 0x001A;		// Slave address - EEPROM control code

   #if (CPU_FRQ_150MHZ)             // Default - For 150MHz SYSCLKOUT
        I2caRegs.I2CPSC.all = 14;   // Prescaler - need 7-12 Mhz on module clk (150/15 = 10MHz)
   #endif
   #if (CPU_FRQ_100MHZ)             // For 100 MHz SYSCLKOUT
     I2caRegs.I2CPSC.all = 9;	    // Prescaler - need 7-12 Mhz on module clk (100/10 = 10MHz)
   #endif

   I2caRegs.I2CCLKL = 100;			// NOTE: must be non zero
   I2caRegs.I2CCLKH = 100;			// NOTE: must be non zero
   I2caRegs.I2CIER.all = 0x24;		// Enable SCD & ARDY interrupts

//   I2caRegs.I2CMDR.all = 0x0020;	// Take I2C out of reset
   I2caRegs.I2CMDR.all = 0x0420;	// Take I2C out of reset		//zq
   									// Stop I2C when suspended

   I2caRegs.I2CFFTX.all = 0x6000;	// Enable FIFO mode and TXFIFO
   I2caRegs.I2CFFRX.all = 0x2040;	// Enable RXFIFO, clear RXFFINT,

}

static Uint16 AIC23Write(int16_t Address,int16_t Data)
{
   if (I2caRegs.I2CMDR.bit.STP == 1)
   {
      return I2C_STP_NOT_READY_ERROR;
   }

   // Setup slave address
   I2caRegs.I2CSAR = 0x1A;

   // Check if bus busy
   if (I2caRegs.I2CSTR.bit.BB == 1)
   {
      return I2C_BUS_BUSY_ERROR;
   }

   // Setup number of bytes to send
   // MsgBuffer + Address
   I2caRegs.I2CCNT = 2;
   I2caRegs.I2CDXR = Address;
   I2caRegs.I2CDXR = Data;
   // Send start as master transmitter
   I2caRegs.I2CMDR.all = 0x6E20;
   return I2C_SUCCESS;

}

void AIC23Init(){
	AIC23Write(0x00,0x00);
	Delay(100);
	AIC23Write(0x02,0x00);
	Delay(100);
	AIC23Write(0x04,0x7f);
	Delay(100);
	AIC23Write(0x06,0x7f);
	Delay(100);
	AIC23Write(0x08,0x14);// MIC input
	Delay(100);
	AIC23Write(0x0A,0x00);
	Delay(100);
	AIC23Write(0x0C,0x00);
	Delay(100);
	AIC23Write(0x0E,0x43);// 16-bit
	Delay(100);
	AIC23Write(0x10,0x0d);// Sampling rate 8k (00001101 0x0d)
	Delay(100);
	AIC23Write(0x12,0x01);// Activate interface
	Delay(100);		//AIC23Init
}

static int16_t __grok_mode_name(char *mode_str, enum Mode *mode_out)
{
    if (!strcmp(mode_str, "MR475"))
        *mode_out = MR475;
    else if (!strcmp(mode_str, "MR515"))
        *mode_out = MR515;
    else if (!strcmp(mode_str, "MR59"))
        *mode_out = MR59;
    else if (!strcmp(mode_str, "MR67"))
        *mode_out = MR67;
    else if (!strcmp(mode_str, "MR74"))
        *mode_out = MR74;
    else if (!strcmp(mode_str, "MR795"))
        *mode_out = MR795;
    else if (!strcmp(mode_str, "MR102"))
        *mode_out = MR102;
    else if (!strcmp(mode_str, "MR122"))
        *mode_out = MR122;
    else
        return -1;
    return 0;
}

void create_wav_header(uint8_t *header, uint32_t data_size) {
//	memcpy(header, 0, 44);
    // RIFF chunk
    memcpy(header, "RIFF", 4);
    uint32_t chunk_size = 36 + data_size;  // Total file size - 8
    header[4] = chunk_size & 0xFF;
    header[5] = (chunk_size >> 8) & 0xFF;
    header[6] = (chunk_size >> 16) & 0xFF;
    header[7] = (chunk_size >> 24) & 0xFF;
    memcpy(header+8, "WAVE", 4);


    memcpy(header+12, "fmt ", 4);
    header[16] = 16;  // fmt chunk size
    header[17] = 0;
    header[18] = 0;
    header[19] = 0;
    header[20] = 1;
    header[21] = 0;   // High byte is 0
    header[22] = 1;   // Mono
    header[23] = 0;
    header[24] = 0x40; // 8000 Hz sampling rate (0x1F40)
    header[25] = 0x1F;
    header[26] = 0;
    header[27] = 0;
    header[28] = 0x80; // Byte rate = 8000*2 = 16000 (0x3E80)
    header[29] = 0x3E;
    header[30] = 0;
    header[31] = 0;
    header[32] = 2;   // Block align = 2 bytes
    header[33] = 0;
    header[34] = 16;  // Bit depth
    header[35] = 0;

    // data sub-chunk
    memcpy(header+36, "data", 4);
    header[40] = data_size & 0xFF;
    header[41] = (data_size >> 8) & 0xFF;
    header[42] = (data_size >> 16) & 0xFF;
    header[43] = (data_size >> 24) & 0xFF;
}

/**
 * Convert 16-bit PCM to 8-bit byte stream
 * @param src 16-bit PCM data source
 * @param dst 8-bit output buffer
 * @param num_samples Number of samples
 */
void convert_16bit_to_8bit(const int16_t *src, uint8_t *dst, uint32_t num_samples) {
    for (uint32_t i = 0; i < num_samples; i++) {
        // Little-endian storage: low byte first, then high byte
        *dst++ = (uint8_t)(src[i] & 0xFF);       // Low byte
        *dst++ = (uint8_t)((src[i] >> 8) & 0xFF); // High byte
    }
}

/**
 * Convert 8-bit byte stream to 16-bit PCM
 * @param src 8-bit input data
 * @param dst 16-bit PCM output buffer
 * @param num_bytes Number of input bytes (must be even)
 */
void convert_8bit_to_16bit(const uint8_t *src, uint16_t *dst, uint32_t num_bytes) {
    uint32_t num_samples = num_bytes / 2;
    for (uint32_t i = 0; i < num_samples; i++) {
        // Big-endian combination: High byte + Low byte
        dst[i] = (uint16_t)((src[0] << 8) | src[1]);
        src += 2;
    }
}


static void __amr_encoder_create(struct amr_encoder_state *st, int16_t dtx, int16_t use_vad2)
{
    amr_encoder_reset(st, dtx, use_vad2);
}

/* length must be less than or equal to 320 */
static void __wavrd_get_pcm_block(uint8_t **bytes, uint16_t length, int16_t *pcm)
{
    uint16_t i;

    length >>= 1;
    for (i = 0; i < length; i++) {
        pcm[i] = (*bytes)[0] | ((*bytes)[1] << 8);
        *bytes += 2;
    }
    while (i < 160) pcm[i++] = 0;
}

/**
 * AMR audio encoder wrapper
 * @param wav_data Input WAV audio data
 * @param wav_len  Length of input WAV data (bytes)
 * @param amr_buf  Output AMR data buffer
 * @param amr_buf_size Output buffer size (bytes)
 * @param amr_len  Output parameter: Actual length of encoded AMR data (bytes)
 * @return 0 on success, negative value indicates error code
 */

int16_t amr_encode_wav(const uint8_t *wav_data, uint32_t wav_len,
                  uint8_t *amr_buf, uint16_t amr_buf_size,
                  uint16_t *amr_len)
{
    char *mode_arg;
    enum Mode mode_val;
    int16_t pcm[160];
    struct amr_param_frame frame;
    uint8_t out_bytes[AMR_IETF_MAX_PL] = {0};
    uint32_t rc;
    int16_t dtx = 0, vad2 = 0;
    unsigned nbytes;

    uint16_t buf_offset = 0;                    // Current write position in buffer

    extern struct amr_encoder_state __encode_st;

    /* Parameter setup */
    mode_arg = "MR515";
    vad2 = 1;
    dtx = 1;

    /* Encoding */
    /* Get mode name */
     rc = __grok_mode_name(mode_arg, &mode_val);
//     if (rc < 0) {
//      printf("error: invalid mode argument \"%s\"\n",
//          mode_arg);
//      return -1;
//     }
    /* Get audio data from WAV and save to buffer; analyze audio format to see if it meets requirements */
    uint8_t *ptr_data;
    rc = __wav_read_open(wav_data, &ptr_data, wav_len);
//    printf("rc : %d\n", rc);
//    if(rc < 0){
//    	printf("WAV parse error: %d\n", rc);
//        return -1;
//    }

     /* Initialize encoder */
     __amr_encoder_create(&__encode_st, dtx, vad2);

     /* Initialize AMR header */
     const uint8_t amr_header[] = {
         '#', '!', 'A', 'M', 'R', '\n'
     };
     memset(amr_buf,0,amr_buf_size);
     memcpy(amr_buf, amr_header, AMR_IETF_HDR_LEN);
     buf_offset = AMR_IETF_HDR_LEN;

     /* Encode audio and save results to buffer; read 320 bytes each time until finished */
     // Check remaining bytes, read at most 320 bytes each time for encoding
     // If remaining bytes are 0, break

     uint8_t *pcm_ptr = ptr_data;
     while(1){
         if(rc > 320){
             __wavrd_get_pcm_block(&pcm_ptr, 320, pcm);
             rc -= 320;
         }
         else if(rc > 0){
             __wavrd_get_pcm_block(&pcm_ptr, rc, pcm);
             rc = 0;
         }
         else{
             break;
         }
         amr_encode_frame(&__encode_st, mode_val, pcm, &frame);

         nbytes = amr_frame_to_ietf(&frame, out_bytes);

         /* Copy encoded output data out_bytes to the final buffer */
         if (buf_offset + nbytes > amr_buf_size) {
//             printf("buffer overflow\n");
             nbytes = amr_buf_size - buf_offset;
             memcpy(amr_buf + buf_offset, out_bytes, nbytes);
             buf_offset += nbytes;
             break;
//             return -1;  // Buffer overflow
         }
         memcpy(amr_buf + buf_offset, out_bytes, nbytes);
         buf_offset += nbytes;
     }
     *amr_len = buf_offset;

    return 0;  // Success
}

int16_t amr_encode_pcm16(const int16_t *pcm_data, uint32_t sample_count,
                    uint8_t *amr_buf, uint16_t amr_buf_size,
                    uint16_t *amr_len)
{
    char *mode_arg;
    enum Mode mode_val;
    int16_t pcm[160];
    struct amr_param_frame frame;
    uint8_t out_bytes[AMR_IETF_MAX_PL] = {0};
    int16_t dtx = 0, vad2 = 0;
    uint16_t buf_offset = 0;
    uint32_t sample_offset = 0;
    unsigned nbytes;
    int16_t i;

    extern struct amr_encoder_state __encode_st;

    mode_arg = "MR515";
    vad2 = 1;
    dtx = 1;

    if (__grok_mode_name(mode_arg, &mode_val) < 0) {
        return -1;
    }

    __amr_encoder_create(&__encode_st, dtx, vad2);

    memset(amr_buf, 0, amr_buf_size);
    memcpy(amr_buf, amr_file_header_magic, AMR_IETF_HDR_LEN);
    buf_offset = AMR_IETF_HDR_LEN;

    while (sample_offset < sample_count) {
        uint32_t remain = sample_count - sample_offset;
        if (remain >= 160U) {
            memcpy(pcm, &pcm_data[sample_offset], 160U * sizeof(int16_t));
            sample_offset += 160U;
        } else {
            for (i = 0; i < (int16_t)remain; i++) {
                pcm[i] = pcm_data[sample_offset + (uint32_t)i];
            }
            while (i < 160) {
                pcm[i++] = 0;
            }
            sample_offset = sample_count;
        }

        amr_encode_frame(&__encode_st, mode_val, pcm, &frame);
        nbytes = amr_frame_to_ietf(&frame, out_bytes);

        if (buf_offset + nbytes > amr_buf_size) {
            UARTa_SendStringAndNumber("error: AMR buffer overflow, used=", buf_offset, "\r\n");
            UARTa_SendStringAndNumber("error: AMR frame bytes needed=", nbytes, "\r\n");
            *amr_len = buf_offset;
            return -1;
        }

        memcpy(amr_buf + buf_offset, out_bytes, nbytes);
        buf_offset += nbytes;
    }

    *amr_len = buf_offset;
    return 0;
}


void __amr_decoder_create(struct amr_decoder_state *st)
{
    amr_decoder_reset(st);
}

void __write_pcm_to_wav(uint8_t **out_buf, const int16_t *pcm, uint32_t *data_length)
{
    unsigned n;

    for (n = 0; n < 160; n++) {
        *(*out_buf)++ = pcm[n] & 0xFF;
        *(*out_buf)++ = (pcm[n] >> 8) & 0xFF;
    }
    *data_length += 320;
}

void __write_string(uint8_t **data_buf, const char *str) {
    *(*data_buf)++ = str[0];
    *(*data_buf)++ = str[1];
    *(*data_buf)++ = str[2];
    *(*data_buf)++ = str[3];
}

void __write_int32(uint8_t **data_buf, int32_t value) {
    *(*data_buf)++ = ((value >>  0) & 0xff);
    *(*data_buf)++ = ((value >>  8) & 0xff);
    *(*data_buf)++ = ((value >>  16) & 0xff);
    *(*data_buf)++ = ((value >>  24) & 0xff);
}

void __write_int16(uint8_t **data_buf, int16_t value) {
    *(*data_buf)++ = ((value >>  0) & 0xff);
    *(*data_buf)++ = ((value >>  8) & 0xff);
}

void __write_header(uint8_t *ptr_data_buf, uint32_t length) {
    uint16_t bytes_per_frame;
    uint32_t bytes_per_sec;
    __write_string(&ptr_data_buf, "RIFF");
    __write_int32(&ptr_data_buf, 4 + 8 + 16 + 8 + length);
    __write_string(&ptr_data_buf, "WAVE");

    __write_string(&ptr_data_buf, "fmt ");
    __write_int32(&ptr_data_buf, 16);

    bytes_per_frame = 16/8*1; //ww->bits_per_sample/8*ww->channels
    bytes_per_sec   = bytes_per_frame*8000;
    __write_int16(&ptr_data_buf, 1);                   // Format
    __write_int16(&ptr_data_buf, 1);        // Channels
    __write_int32(&ptr_data_buf, 8000);     // Samplerate
    __write_int32(&ptr_data_buf, bytes_per_sec);       // Bytes per sec
    __write_int16(&ptr_data_buf, bytes_per_frame);     // Bytes per frame
    __write_int16(&ptr_data_buf, 16); // Bits per sample

    __write_string(&ptr_data_buf, "data");
    __write_int32(&ptr_data_buf, length);
}

/**
 * AMR audio decoder wrapper
 * @param amr_data Input AMR audio data
 * @param amr_len  Length of input AMR data (bytes)
 * @param wav_buf  Output WAV data buffer
 * @param wav_buf_size Output buffer size (bytes)
 * @param wav_len  Output parameter: Actual length of decoded WAV data (bytes)
 * @return 0 on success, negative value indicates error code
 */

int16_t amr_decode_wav(const uint8_t *amr_data, uint16_t amr_len,
                  uint8_t *wav_ptr, uint32_t wav_buf_size,
                  uint32_t *wav_len)
{
    int16_t pcm[160];
    struct amr_param_frame frame;
    uint8_t out_bytes[AMR_IETF_MAX_PL] = {0};

    uint16_t buf_offset = 0;                    // Current write position in buffer
    int16_t rc = 0;

    extern struct amr_decoder_state __decode_st;

    /* Decoding */
    if(memcmp(amr_data, amr_file_header_magic, AMR_IETF_HDR_LEN)){
        UARTa_SendString("file is not in IETF AMR format\n");
        return -1;
    }
    else{
        buf_offset += AMR_IETF_HDR_LEN;
    }

    /* Initialize decoder */
    __amr_decoder_create(&__decode_st);

    uint8_t *ptr_data = wav_ptr+44;
    uint32_t data_length = 0;
    for (;;) {
        if(buf_offset >= amr_len)
            break;
        rc = amr_data[buf_offset++];
        out_bytes[0] = rc;
        rc = amr_ietf_grok_first_octet(out_bytes[0]);
        if (rc < 0) {
            UARTa_SendString("error: file contains invalid AMR data\n");
            return -1;
        }

        if (buf_offset + rc > amr_len) {
            UARTa_SendString("warning: incomplete AMR frame at EOF\n");
            break;
        }

        memcpy(out_bytes+1, &amr_data[buf_offset], rc*sizeof(uint8_t));
        buf_offset += rc;

        amr_frame_from_ietf(out_bytes, &frame);
        amr_decode_frame(&__decode_st, &frame, pcm);
        if((data_length + 44 + 320) > wav_buf_size){
            UARTa_SendString("error: wav buffer overflow\n");
            return -1;
        }
        __write_pcm_to_wav(&ptr_data, pcm, &data_length);
    }
    ptr_data = wav_ptr;
    __write_header(ptr_data, data_length);

    *wav_len = data_length+44;

    return 0;
}

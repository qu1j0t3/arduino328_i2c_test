enum{
   MT_START_SENT = 0x08, // see datasheet Table 22-2
   MT_REP_START_SENT = 0x10,
   MT_SLA_ACK = 0x18,  // after SLA+W
   MT_SLA_NAK = 0x20,  // after SLA+W
   MT_DATA_ACK = 0x28, // after data byte
   MT_DATA_NAK = 0x30, // after data byte
   MT_ARB_LOST = 0x38, // after SLA+W
   MR_SLA_ACK = 0x40,  // after SLA+R
   MR_SLA_NAK = 0x48,  // after SLA+R
   MR_DATA_RCVD_ACK = 0x50,
   MR_DATA_RCVD_NAK = 0x58
};

struct twi_receive_t {
   uint8_t data;
   uint8_t status;
};

void twi_init();
uint8_t twi_send(uint8_t b);
uint8_t twi_command(uint8_t addr, uint8_t cmd);
struct twi_receive_t twi_receive_byte_nak(uint8_t addr);

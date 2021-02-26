#define RST_PIN 13
#define PROG_PIN 12
#define TPI_CLK_PIN 11
#define TPI_DATA_PIN 10

#define OP_SKEY    0b11100000
#define OP_SLDCS   0b10000000
#define OP_SSTCS   0b11000000
#define OP_SIN     0b00010000
#define OP_SOUT    0b10010000
#define OP_SSTPR_L 0b01101000
#define OP_SSTPR_H 0b01101001
#define OP_SLD     0b00100000
#define OP_SST     0b01100000

const unsigned char NVM_KEY[] = {0x12, 0x89, 0xAB, 0x45, 0xCD, 0xD8, 0x88, 0xFF};

void tpi_enable() {
  digitalWrite(PROG_PIN, 1);
  delay(100);
  digitalWrite(RST_PIN, 0);
  for (int i = 0; i < 16; i++) {
    tpi_send_bit(1);
  }
}

void tpi_send_bit(unsigned char c) {
  digitalWrite(TPI_CLK_PIN, 0);
  digitalWrite(TPI_DATA_PIN, c);
  digitalWrite(TPI_CLK_PIN, 1);
}

void tpi_clock_pulse() {
  digitalWrite(TPI_CLK_PIN, 0);
  digitalWrite(TPI_CLK_PIN, 1);
}

void tpi_send(unsigned char c) {
  digitalWrite(TPI_CLK_PIN, 0);
  tpi_send_bit(0);
  char parity = 0;
  for (int i = 0; i < 8; i++) {
    char nextbit = c & (1 << i) ? 1 : 0;
    parity ^= nextbit;
    tpi_send_bit(nextbit);
  }
  
  tpi_send_bit(parity);
  tpi_send_bit(1);
  tpi_send_bit(1);
  digitalWrite(TPI_CLK_PIN, 0);
  digitalWrite(TPI_DATA_PIN, 1);
}

unsigned char tpi_get() {
  pinMode(TPI_DATA_PIN, INPUT);
  tpi_clock_pulse();
  tpi_clock_pulse();
  tpi_clock_pulse();
  unsigned char out = 0; 
  unsigned char parity = 0;
  for (int i = 0; i < 8; i++) {
    tpi_clock_pulse();
    char c = digitalRead(TPI_DATA_PIN);
    parity ^= c;
    out |= (c << i);
  }
  tpi_clock_pulse();
  parity ^= digitalRead(TPI_DATA_PIN);
  if (parity) {
    Serial.println("Parity error!!");
    Serial.println((int)out);
  }
  tpi_clock_pulse();
  tpi_clock_pulse();
  pinMode(TPI_DATA_PIN, OUTPUT);
  return out;
}

void enable_nvm() {
  tpi_send(OP_SKEY);
  for (int i = 0; i < 8; i++) {
    tpi_send(NVM_KEY[7-i]);
  }
  tpi_send(OP_SLDCS | 0x0);
  if (tpi_get() != 2) {
    Serial.println("Failed to enable NVM programming");  
  }
}

void set_no_extra_get_stuff() {
  tpi_send(OP_SSTCS | 0x02);
  tpi_send(0x7);
  tpi_send(OP_SLDCS | 0x02);
  if (tpi_get() != 7) {
    Serial.println("Failed to set get guud");
  }
}

unsigned char serial_in_addr_to_op_code(unsigned char opcode, unsigned char addr) {
  return (opcode | ((addr & 0b110000) << 1)) | (addr & 0b1111);
}

unsigned char read_io(unsigned char addr) {
  tpi_send(serial_in_addr_to_op_code(OP_SIN, addr));
  return tpi_get();
}

void write_io(unsigned char addr, unsigned char val) {
  tpi_send(serial_in_addr_to_op_code(OP_SOUT, addr));
  tpi_send(val);
}

void pointer_load(unsigned int addr) {
  unsigned char high = addr >> 8;
  unsigned char low = addr & 0xFF;
  tpi_send(OP_SSTPR_L);
  tpi_send(low);
  tpi_send(OP_SSTPR_H);
  tpi_send(high);
}

unsigned char data_read(char inc) {
  tpi_send(OP_SLD | (inc ? 0b100 : 0));
  return tpi_get();
}

unsigned char data_write(char inc, unsigned char data) {
  tpi_send(OP_SST | (inc ? 0b100 : 0));
  tpi_send(data);
}

void set_rstdisbl(char val) {
  write_io(0x33, 0x14); // should erase next section we write to
  pointer_load(0x3F41);
  data_write(0, 0);
  wait_nvm_ready();
  if (val) {
    pointer_load(0x3F40);
    data_write(0, 1); 
  }
}

volatile void wait_nvm_ready() {
  unsigned char res = 1;
  while (res) {
    res = read_io(0x32);
  }
}

void erase_chip() {
  write_io(0x33, 0x10); // should erase ship
  pointer_load(0x4001);
  data_write(0, 0);
  wait_nvm_ready();
}

void write_flash(unsigned char lower, unsigned char upper) {
  // does not load address, increments address +2
  // use pointer_load for that 
  write_io(0x33, 0x1D);
  data_write(1, lower);
  data_write(1, upper);
  wait_nvm_ready();
}


void start() {
  digitalWrite(TPI_DATA_PIN, 0);
  tpi_enable();
  set_no_extra_get_stuff();
  enable_nvm();
  erase_chip();
  pointer_load(0x4000);
}

void finish() {
  tpi_send(OP_SSTCS | 0x0);
  tpi_send(0);
  digitalWrite(PROG_PIN, 0);
}

void dump_mem() {
  pointer_load(0x4000);
  for (int i = 0; i < 1024; i++) {
    unsigned char c = data_read(1);
    if (c < 16) {
      Serial.print("0");
    }
    Serial.print(c, HEX);
  }
}

void setup() {
  pinMode(RST_PIN, OUTPUT);
  pinMode(PROG_PIN, OUTPUT);
  pinMode(TPI_CLK_PIN, OUTPUT);
  pinMode(TPI_DATA_PIN, OUTPUT);
  Serial.begin(115200);
}

unsigned char buf[4];
int i = 0;

unsigned char to_char(unsigned char c) {
  if (c <= '9') {
    return c - '0';
  }
  return 10 + c - 'A';
}

unsigned char to_byte(unsigned char c1, unsigned char c2) {
  return 16 * to_char(c1) + to_char(c2);
}

void loop() {
  if (Serial.available() > 0) {
    char com = Serial.read();
    Serial.write(com);
    
    if (com == 'S') {
      start();
      i = 0;
    } else if (com == 'V') {
      set_rstdisbl(1);
    } else if (com == 'v') {
      set_rstdisbl(0);
    } else if (com == 'X') {
      finish();
    } else if (com == 'R') {
      dump_mem();
      pointer_load(0x4000);
    } else {
      buf[i++] = com;
      if (i == 4) {
        write_flash(to_byte(buf[0], buf[1]), to_byte(buf[2], buf[3]));
        i = 0;
      }
    }
  }
}

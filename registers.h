// class for register 

class Register {
  private:
    bool valid;
    int value;
    bool zeroFlag;
  public:
    Register();
    bool read_valid_bit();
    int read_value();
    void write_valid_bit(bool);
    void write_value(int);
    void reset();
    bool read_z();
};
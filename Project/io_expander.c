#include "eeprom.h"
#include "io_expander.h"

int32_t i2c_base = IO_EXPANDER_I2C_BASE;

static i2c_status_t io_expander_wait_for_write () 
{
	
	i2c_status_t status;
  
  if( !i2cVerifyBaseAddr(i2c_base) )
  {
    return  I2C_INVALID_BASE;
  }

  // Set the I2C address to be the IO Expander and in Write Mode
  status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_WRITE);

  // Poll while the device is busy.  The  MCP23017_DEV_ID will not ACK
  // writing an address while the write has not finished.
  do 
  {
    // The data we send does not matter.  This has been set to 0x00, but could
    // be set to anything
    status = i2cSendByte( i2c_base, 0x00, I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP);
    
    // Wait for the address to finish transmitting
    while ( I2CMasterBusy(i2c_base)) {};
    
    // If the address was not ACKed, try again.
  } while (I2CMasterAdrAck(i2c_base) == false);

  return  status;
}


bool io_expander_init(void) 
{
	
  if(gpio_enable_port(IO_EXPANDER_GPIO_BASE) == false)
  {
    return false;
  }
  
  // Configure SCL 
  if(gpio_config_digital_enable(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_alternate_function(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_port_control(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SCL_PCTL_M, IO_EXPANDER_I2C_SCL_PIN_PCTL)== false)
  {
    return false;
  }
    

  
  // Configure SDA 
  if(gpio_config_digital_enable(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_open_drain(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_alternate_function(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PIN)== false)
  {
    return false;
  }
    
  if(gpio_config_port_control(IO_EXPANDER_GPIO_BASE, IO_EXPANDER_I2C_SDA_PCTL_M, IO_EXPANDER_I2C_SDA_PIN_PCTL)== false)
  {
    return false;
  }
	
	
	// Push buttons input and pull up
	io_expander_write_reg(MCP23017_IODIRB_R, 0xF); // 1 for input
	io_expander_write_reg(MCP23017_DEFVALB_R, 0xF); // set default 
	io_expander_write_reg(MCP23017_GPPUB_R, 0xF); // pull ups
	io_expander_write_reg(MCP23017_GPINTENB_R, 0xF); // interrupts
	
	// Configure LEDS output
	io_expander_write_reg(MCP23017_IODIRA_R, 0x00); // 0 for Output
	
    
  //  Initialize the I2C peripGPIOheral
  if( initializeI2CMaster(IO_EXPANDER_I2C_BASE)!= I2C_OK)
  {
    return false;
  }
  
  return true;
	
}

void io_expander_write_reg(uint8_t reg, uint8_t data) 
{
	i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while ( I2CMasterBusy(i2c_base)) {};

  //==============================================================
  // Set the I2C address to be the IO Expander
  //==============================================================
	status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_WRITE);
	if ( status != I2C_OK )
  {
    // return status;
  }
  
  // If the IO Expander is still writing the last byte written, wait
  io_expander_wait_for_write();
  
  //==============================================================
  // Send the Upper byte of the address
  //==============================================================
	// Also sends control 
	i2cSendByte(
     i2c_base,                   // Device OP
     reg,                        // ADDR
     I2C_MCS_START | I2C_MCS_RUN // S
	);	                           // run? (write sent above) 
	
	
  //==============================================================
  // Send the Lower byte of the address
  //==============================================================
  /*
	i2cSendByte(
     i2c_base,
     address & 0xFF,
     I2C_MCS_RUN
  );
	*/
	
	
  //==============================================================
  // Send the Byte of data to write
  //==============================================================
	i2cSendByte(
     i2c_base,
     data,
     I2C_MCS_RUN | I2C_MCS_STOP
	);


  //return status;
}


uint8_t io_expander_read_reg(uint8_t reg) 
{
	 i2c_status_t status;
	 uint8_t data;
  
  // Before doing anything, make sure the I2C device is idle
  while ( I2CMasterBusy(i2c_base)) {};

  // If the IO Expander is still writing the last byte written, wait
  io_expander_wait_for_write();

  //==============================================================
  // Set the I2C slave address to be the IO Expander and in Write Mode
  //==============================================================	
	status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_WRITE);
	if ( status != I2C_OK )
  {
    return status;
  }
	
  //==============================================================
  // Send the Upper byte of the address
  //==============================================================
	i2cSendByte(
     i2c_base,
     reg,
     I2C_MCS_START | I2C_MCS_RUN
	);	
	
	
  //==============================================================
  // Send the Lower byte of the address
  //==============================================================
	/*
	i2cSendByte(
			 i2c_base,
			 address & 0xFF,
			 I2C_MCS_RUN
		);
	*/

  //==============================================================
  // Set the I2C slave address to be the IO Expander and in Read Mode
  //==============================================================
	status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_READ);
	if ( status != I2C_OK )
  {
    return status;
  }

  //==============================================================
  // Read the data returned by the IO Expander
  //==============================================================
  
	i2cGetByte( 
     i2c_base,
     &data,
     I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP
	);
	
	
  return data;
}

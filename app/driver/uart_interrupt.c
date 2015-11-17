#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "osapi.h"
#include "driver/uart_register.h"
#include "user_config.h"
#include "mem.h"
#include "os_type.h"
#include "util/linked_list.h"

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

#define UART_RX_INTR_DISABLE(uart) CLEAR_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA)
#define UART_RX_INTR_ENABLE(uart) SET_PERI_REG_MASK(UART_INT_ENA(uart), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTR_DISABLE(uart) CLEAR_PERI_REG_MASK(UART_INT_ENA(uart), UART_TXFIFO_EMPTY_INT_ENA)
#define UART_TX_INTR_ENABLE(uart) SET_PERI_REG_MASK(UART_CONF1(uart), (UART_TX_EMPTY_THRESH_VAL & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S);	\
    						   SET_PERI_REG_MASK(UART_INT_ENA(uart), UART_TXFIFO_EMPTY_INT_ENA)

#define UART_RESET_FIFO(uart) SET_PERI_REG_MASK(UART_CONF0(uart), UART_RXFIFO_RST | UART_TXFIFO_RST);	\
    					   CLEAR_PERI_REG_MASK(UART_CONF0(uart), UART_RXFIFO_RST | UART_TXFIFO_RST)

#define UART_CLEAR_ALL_INTR(uart) WRITE_PERI_REG(UART_INT_CLR(uart), 0xffff)
#define UART_CLEAR_INTR(uart,INTERRUPT) WRITE_PERI_REG(UART_INT_CLR(uart), INTERRUPT)
#define UART_INTERRUPT_IS(uart,INTERRUPT) INTERRUPT == (READ_PERI_REG(UART_INT_ST(uart)) & INTERRUPT)

#define UART_RX_FIFO_COUNT(uart) (READ_PERI_REG(UART_STATUS(uart))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT
#define UART_TX_FIFO_COUNT(uart) (READ_PERI_REG(UART_STATUS(uart))>>UART_TXFIFO_CNT_S)&UART_TXFIFO_CNT

#define UART0_READ_CHAR() READ_PERI_REG(UART_FIFO(UART0)) & 0xFF
#define UART_WRITE_CHAR(uart,c) WRITE_PERI_REG(UART_FIFO(uart), c)

static uart0_data_received_callback_t uart0_data_received_callback=NULL;

LOCAL void uart_rx_intr_handler(void *para);

LOCAL void uart_transmit(uint8_t uart);

#define TX_ITEM_LEN 128

struct tx_item{
	uint8_t data[TX_ITEM_LEN];
	int len;
};

linked_list tx_list[2];
os_timer_t tx_timer[2];

//UART TRANSMIT ------------------------------------------------------------
STATUS uart_tx_one_char(uint8_t uart, uint8_t TxChar)
{
    while (true){
        uint8_t fifo_cnt = UART_TX_FIFO_COUNT(uart);
        if (fifo_cnt < 126) {
            break;
        }
    }
    UART_WRITE_CHAR(uart,TxChar);
    return OK;
}

STATUS uart_tx_one_char_no_wait(uint8_t uart, uint8_t TxChar)
{
    uint8_t fifo_cnt = UART_TX_FIFO_COUNT(uart);
    if (fifo_cnt < 126) {
        UART_WRITE_CHAR(uart,TxChar);
    }
    return OK;
}

static void tx_timer_timeout(void *arg){

	//NODE_DBG("u tout");

	if(arg==(void*)0){
		uart_transmit(0);
	}
	else{
		uart_transmit(1);
	}

}

void uart_write(uint8_t uart,uint8_t *data,int len){

	linked_list *list = &tx_list[uart];

	uint8_t fifo_cnt = UART_TX_FIFO_COUNT(uart);
    if (fifo_cnt > len && list->size==0) { 
    	//we can write on uart buffer
    	while(len>0){
    		UART_WRITE_CHAR(uart,*data);
    		data++;
    		len--;
    	}
        
        return;
    }

    //if fifo is full, buffer on our own	
		
	while(len>0){

		struct tx_item *tx = (struct tx_item *)list_get_first(list);

		if(tx==NULL || tx->len >= TX_ITEM_LEN){
			tx = (struct tx_item*)os_zalloc(sizeof(struct tx_item));
			list_add_first(list,tx);
		}

		int remLen = TX_ITEM_LEN-tx->len;
		if(len <= remLen){
			os_memcpy(tx->data+tx->len,data,len);
			tx->len+=len;
			len=0;
		}
		else{
			os_memcpy(tx->data+tx->len,data,remLen);
			tx->len+=remLen;
			len-=remLen;
			data+=remLen;
		}
				
	}
	
	os_timer_disarm(&tx_timer[uart]);
	os_timer_arm(&tx_timer[uart], 20, 0);
}

void uart_write_char(uint8_t uart,char c){
	uart_write(uart,&c,1);		
}

void uart_write_string(uint8_t uart,const char *s){
	int str_len = os_strlen(s);
	uart_write(uart,(uint8_t*)s,str_len);
}


void ICACHE_FLASH_ATTR uart_config(uint8_t uart_no)
{
    if (uart_no == UART1){
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    }else{
        /* rcv_buff size if 0x100 */
        ETS_UART_INTR_ATTACH(uart_rx_intr_handler,  &(UartDev.rcv_buff));
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
		#if UART_HW_RTS
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);   //HW FLOW CONTROL RTS PIN
		    #endif
		#if UART_HW_CTS
		    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_U0CTS);   //HW FLOW CONTROL CTS PIN
		#endif
    }
    uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate));//SET BAUDRATE
    
    WRITE_PERI_REG(UART_CONF0(uart_no), ((UartDev.exist_parity & UART_PARITY_EN_M)  <<  UART_PARITY_EN_S) //SET BIT AND PARITY MODE
                                                                        | ((UartDev.parity & UART_PARITY_M)  <<UART_PARITY_S )
                                                                        | ((UartDev.stop_bits & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)
                                                                        | ((UartDev.data_bits & UART_BIT_NUM) << UART_BIT_NUM_S));
    
    //clear rx and tx fifo,not ready
    UART_RESET_FIFO(uart_no);
    
    if (uart_no == UART0){
        //set rx fifo trigger
        WRITE_PERI_REG(UART_CONF1(uart_no),
        ((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
        #if UART_HW_RTS
	        ((110 & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S) |
	        UART_RX_FLOW_EN |   //enbale rx flow control
        #endif
	        (0x60 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S |
	        UART_RX_TOUT_EN|
        ((0x10 & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S));//wjl 
        #if UART_HW_CTS
        	SET_PERI_REG_MASK( UART_CONF0(uart_no),UART_TX_FLOW_EN);  //add this sentense to add a tx flow control via MTCK( CTS )
        #endif
        SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_TOUT_INT_ENA |UART_FRM_ERR_INT_ENA);
    }else{
        WRITE_PERI_REG(UART_CONF1(uart_no),((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));//TrigLvl default val == 1
    }
    //clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
    //enable rx_interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA);
}
 
LOCAL void uart_transmit(uint8_t uart){

	
	uint8_t tx_fifo_len;
	uint8_t fifo_remain;

	linked_list *list = &tx_list[uart];

	struct tx_item *item=NULL;

process:
	tx_fifo_len = UART_TX_FIFO_COUNT(uart);
	fifo_remain = UART_FIFO_LEN - tx_fifo_len ;
	item = (struct tx_item *)list_remove_last(list);

	if(item!=NULL){

		int i;
		int tx_count = item->len > fifo_remain?fifo_remain:item->len;

		if(fifo_remain >= item->len){
			for(i = 0; i<item->len;i++)
        		UART_WRITE_CHAR(uart,item->data[i]);

        	os_free(item);
        	goto process;
		}
		else{
			for(i=0;i<fifo_remain;i++)
				UART_WRITE_CHAR(uart,item->data[i]);

			os_memmove(item->data, item->data+fifo_remain, item->len - fifo_remain);
			item->len -= fifo_remain;
			list_add_last(list,item); //put back on list			
		}

        if(tx_list->size>0)
			UART_TX_INTR_ENABLE(uart); //enable interrupt so we may send data again

		
	}	

}

LOCAL void uart0_data_received(){

	uint8_t data_len = UART_RX_FIFO_COUNT(0);
	
	if(data_len<=0)
		return;

	if(uart0_data_received_callback!=NULL){

		uint8_t *tmp_data = (uint8_t*)os_malloc(data_len);
		int i;
		uint8_t c;
		for(i=0;i<data_len;i++){
			c = UART0_READ_CHAR();
			
			tmp_data[i]=c;
		}

		uart0_data_received_callback(tmp_data,data_len);
		os_free(tmp_data);

	}

}

LOCAL void uart_rx_intr_handler(void *para)
{    

	if( UART_INTERRUPT_IS(0,UART_FRM_ERR_INT_ST) ){
		//just clear intr
		UART_CLEAR_INTR(0,UART_FRM_ERR_INT_CLR);
		return;
	}
	if( UART_INTERRUPT_IS(1,UART_FRM_ERR_INT_ST) ){
		//just clear intr
		UART_CLEAR_INTR(1,UART_FRM_ERR_INT_CLR);
		return;
	}


	
	if( UART_INTERRUPT_IS(0,UART_RXFIFO_FULL_INT_ST) ){
		
		//got data on rx fifo
		UART_RX_INTR_DISABLE(0); //disable rx interrupt

		uart0_data_received();

		UART_CLEAR_INTR(0,UART_RXFIFO_FULL_INT_CLR); //clear interrupt
		UART_RX_INTR_ENABLE(0); //enable interrupt back
		return;
	}
	
	if( UART_INTERRUPT_IS(0,UART_RXFIFO_TOUT_INT_ST) ){
		
		//got data on uart 0 rx fifo, timeout for fifo full
		UART_RX_INTR_DISABLE(0); //disable rx interrupt

		uart0_data_received();

		UART_CLEAR_INTR(0,UART_RXFIFO_TOUT_INT_CLR); //clear interrupt
		UART_RX_INTR_ENABLE(0); //enable interrupt back
		return;
	}

	if( UART_INTERRUPT_IS(0,UART_TXFIFO_EMPTY_INT_ST) ){

		UART_TX_INTR_DISABLE(0);

		uart_transmit(0);

		UART_CLEAR_INTR(0,UART_TXFIFO_EMPTY_INT_CLR); //clear interrupt
		return;
	}

	if( UART_INTERRUPT_IS(1,UART_TXFIFO_EMPTY_INT_ST) ){

		UART_TX_INTR_DISABLE(1);

		uart_transmit(1);

		UART_CLEAR_INTR(1,UART_TXFIFO_EMPTY_INT_CLR); //clear interrupt
		return;
	}


	if( UART_INTERRUPT_IS(0,UART_RXFIFO_OVF_INT_ST) ){		
		
		UART_CLEAR_INTR(0,UART_RXFIFO_OVF_INT_CLR); //clear interrupt
		return;
	}

    

}

void uart_register_data_callback(uart0_data_received_callback_t callback){

	uart0_data_received_callback=callback;	
}
void uart_clear_data_callback(){

	uart0_data_received_callback=NULL;	
}

void uart1_write_char(char c){
	uart_write_char(1,c);
}
void uart0_write_char(char c){
	uart_write_char(0,c);
}

void uart_init(UartBautRate uart0_br, UartBautRate uart1_br)
{
	//set timers
    os_memset(&tx_timer[0],0,sizeof(os_timer_t));
    os_timer_disarm(&tx_timer[0]);
    os_timer_setfn(&tx_timer[0], (os_timer_func_t *)tx_timer_timeout, 0);

    os_memset(&tx_timer[1],0,sizeof(os_timer_t));
    os_timer_disarm(&tx_timer[1]);
    os_timer_setfn(&tx_timer[1], (os_timer_func_t *)tx_timer_timeout, 1);
    
    init_linked_list(&tx_list[0]);
    init_linked_list(&tx_list[1]);
	


	UartDev.exist_parity = STICK_PARITY_EN;
    UartDev.parity = EVEN_BITS;
    UartDev.stop_bits = ONE_STOP_BIT;
    UartDev.data_bits = EIGHT_BITS;

	UartDev.baut_rate = uart0_br;
    uart_config(UART0);
    UartDev.baut_rate = uart1_br;
    uart_config(UART1);
    ETS_UART_INTR_ENABLE();

#if(DEBUG_UART==0)    
    os_install_putc1((void *)uart0_write_char);
#else
    os_install_putc1((void *)uart1_write_char);
#endif
}
#ifndef __SERIAL_X2_H__
#define __SERIAL_X2_H__

/* Uart enable register bits define */
#define X2_ENR_EN 		(1U << 0)	/* uart enabled */
#define X2_ENR_RX_EN	(1U << 1)	/* RX enabled */
#define X2_ENR_TX_EN	(1U << 2)	/* TX enabled */

/* Uart line control register bits define */
#define	X2_LCR_BITS 	(1U << 0)
#define X2_LCR_STOP		(1U << 1)
#define X2_LCR_PEN 		(1U << 2)
#define X2_LCR_EPS		(1U << 3)
#define X2_LCR_SP		(1U << 4)
#define X2_LCR_CTS_EN	(1U << 5)
#define X2_LCR_RTS_EN	(1U << 6)
#define X2_LCR_SFC_EN	(1U << 7)

/* Uart modem control register */
#define X2_MCR_DTRN			(1U << 0)
#define X2_MCR_RTSN			(1U << 1)
#define X2_MCR_LMD_MASK		0x3
#define X2_MCR_LMD(x)		(((x) & X2_MCR_LMD_MASK) << 2U)
#define X2_MCR_XON_MASK		0xFF
#define X2_MCR_XON(x)		(((x) & X2_MCR_XON_MASK) << 8U)
#define X2_MCR_XOFF_MASK	0xFF
#define X2_MCR_XOFF(x)		(((x) & X2_MCR_XOFF_MASK) << 16U)

/* Uart test control register */
#define X2_TCR_LB			(1U << 0)
#define X2_TCR_FFE			(1U << 1)
#define X2_TCR_FPE			(1U << 2)
#define X2_TCR_BRK			(1U << 3)

/* Uart fifo configuration register */
#define X2_FCR_RDMA_EN		(1U << 0)
#define X2_FCR_TDMA_EN		(1U << 1)
#define X2_FCR_RFRST		(1U << 2)
#define X2_FCR_TFRST		(1U << 3)
#define X2_FCR_RFTRL_MASK	0x1F
#define X2_FCR_RFTRL(x)		(((x) & X2_FCR_RFTRL_MASK) << 16U)
#define X2_FCR_TFTRL_MASK	0x1F
#define X2_FCR_TFTRL(x)		(((x) & X2_FCR_TFTRL_MASK) << 24U)

/* Uart line status register bits define */
#define X2_LSR_TX_EMPTY		(1U << 12)
#define X2_LSR_TF_EMPTY 	(1U << 11)
#define X2_LSR_TF_TLR 		(1U << 10)
#define X2_LSR_TXRDY 		(1U << 9)
#define X2_LSR_TXBUSY 		(1U << 8)
#define X2_LSR_RF_TLR 		(1U << 7)
#define X2_LSR_RX_TO 		(1U << 6)
#define X2_LSR_RX_OE		(1U << 5)
#define X2_LSR_B_INT		(1U << 4)
#define X2_LSR_F_ERR		(1U << 3)
#define X2_LSR_P_ERR		(1U << 2)
#define X2_LSR_RXRDY 		(1U << 1)
#define X2_LSR_RXBUSY 		(1U <<0)

/* uart baud rate register bits define */
#define X2_BCR_MODE_MASK		0x3
#define X2_BCR_MODE(x) 			(((x) & X2_BCR_MODE_MASK) << 28U)
#define X2_BCR_DIV_FRAC_MASK	0x3FF
#define X2_BCR_DIV_FRAC(x) 		(((x) & X2_BCR_DIV_FRAC_MASK) << 16)
#define X2_BCR_DIV_INT_MASK		0xFFFF
#define X2_BCR_DIV_INT(x) 		((x) & X2_BCR_DIV_INT_MASK)

#define BCR_LOW_MODE 	16
#define BCR_MID_MODE 	8
#define BCR_HIGH_MODE 	4

struct x2_uart_regs {
	u32	rdr_reg;			/* 0x00 rxd register */
	u32	tdr_reg;			/* 0x04 txd register */
	u32	lcr_reg;			/* 0x08 line control register */
	u32	enr_reg;			/* 0x0C uart enable register*/
	u32	bcr_reg;			/* 0x10 uart BAUD rate register*/
	u32	mcr_reg;			/* 0x14 uart mode control register */
	u32	tcr_reg;			/* 0x18 uart test control register */
	u32	fcr_reg;			/* 0x1C uart fifo ctrl register */
	u32	lsr_reg;			/* 0x20 uart line status register register */
	u32	msr_reg;			/* 0x24 uart modem status register */
	u32 rxaddr_reg;		/* 0x28 uart receive dma base  addr register*/
	u32	rxsize_reg;		/* 0x2C uart receive dma size register */
	u32	rxdma_reg;		/* 0x30 uart receive dma control register */
	u32 txaddr_reg;		/* 0x34 uart transmit dma base  addr register*/
	u32	txsize_reg;		/* 0x38 uart transmit dma size register */
	u32	txdma_reg;		/* 0x3C uart transmit dma control register */
	u32	spnd_reg;		/* 0x40 uart interrupt source pending  register */
	u32 imask_reg;		/* 0x44 uart interrupt mask register */
	u32 isetmask_reg;	/* 0x48 uart interrupt set mask register */
	u32 iunmask_reg;	/* 0x4c uart interrupt unmask register */
};

#endif /* __SERIAL_X2_H__ */


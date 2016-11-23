	.arch msp430g2553
	.text

	.global playCollisionSoundOnBar
	.global playCollisionSoundOnFence
	.global resetSound
	
playCollisionSoundOnBar:
	mov #2000, &0x0172
	mov #2000, &0x0174
	rra &0x0174
	ret
playCollisionSoundOnFence:
	mov #1000, &0x0172
	mov #1000, &0x0174
	rra &0x0174
	ret
resetSound:
	mov #0, &0x0172
	mov #0, &0x0174
	rra &0x0174
	ret

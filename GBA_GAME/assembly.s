.global bulletfired
bulletfired:
	cmp r0, #1
	beq .next
	b .fail
.next:
	cmp r1, #1
	beq .fail
	mov r0, #1
	mov pc, lr
.fail:
	mov r0, #0
	mov pc, lr


.global brain1
brain1:
	cmp r0, #0
	bne .fail
	cmp r1, r2
	bge .fail
	mov r0, #1
	mov pc, lr

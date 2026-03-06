# test_addi.S
.global _start
.text

_start:
    # Inicializamos registros
    li x5, 0          # x5 = 0
    li x6, 10         # x6 = 10 (Nuestra meta)

    # Probamos ADDI
    addi x5, x5, 4    # x5 = 4
    addi x5, x5, 6    # x5 = 10

    # Comparamos si x5 y x6 son iguales usando un salto (Branch)
    beq x5, x6, test_pass

test_fail:
    # Si falla, ponemos un código de error en x10 (a0) y llamamos a ECALL
    li x10, 1         
    ecall

test_pass:
    # Si pasa, ponemos 0 en x10 (a0) indicando éxito y llamamos a ECALL
    li x10, 0         
    ecall
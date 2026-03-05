#include <iostream>
#include <vector>
#include <iomanip>
#include "MMU.h"
#include "CPU.h"

int main()
{
    // 1. Creamos 1MB de RAM
    MMU memoria(1024 * 1024);

    // 2. Creamos nuestro mini-programa compilado (Little-Endian)
 
     std::vector<uint8_t> programa = {
        // 1. LUI x5, 0x12345 (x5 = 0x12345000)
        0xb7, 0x52, 0x34, 0x12, 
        
        // 2. ADDI x5, x5, 15  (x5 = 0x1234500F)
        0x93, 0x82, 0xf2, 0x00,
        
        // 3. SW x5, 4(x2) -> Guarda el valor de x5 en la dirección (Stack Pointer + 4)
        // Código máquina: 0x00512223
        0x23, 0x22, 0x51, 0x00 
    };
  

    // 3. Cargamos el programa en la dirección 0x0 (donde empieza el PC)
    memoria.loadBinary(programa, 0);

    // 4. Conectamos la CPU a la memoria
    CPU cpu(memoria);

    // 5. Ejecutamos dos pasos (porque tenemos 2 instrucciones)
    std::cout << "Ejecutando instruccion 1..." << "\n";
    cpu.step(); // Ejecuta LUI x5
    
    std::cout << "Ejecutando instruccion 2..." << "\n";
    cpu.step(); // Ejecuta LUI x6


    std::cout << "Ejecutando instruccion 3..." << "\n";
    cpu.step(); // Ejecuta LUI x6


    std::cout << "¡Test finalizado sin crasheos!" << "\n";

    return 0;
}
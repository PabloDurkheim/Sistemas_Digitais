# PROJETO - ALIMENTADOR DE PETS -
## Parcialmente em baixo nível (acessando registradores)
### Objetivo: Proporcionar uma alimentação automatizada e controlada para animais de estimação, oferecendo flexibilidade ao tutor para configurar as quantidades de ração fornecidas por vez.
### Funcionalidades:
 - Configuração da quantidade de
ração a ser liberada;
- Configuração da quantidade de
vezes que a ração será liberada
por dia;
- Liberação da ração quando o pote
estiver vazio, conforme
configuração do tutor

### Dispositivos:
- Arduino MEGA 2560: mestre
- Arduino UNO: escravo

### Sobre o dispositivo:
Inicialmente, o Arduino Mega 2560 foi configurado como mestre, responsável pela interface
com o usuário por meio de um display LCD 16x2 e botões para configuração. O Arduino
Uno, configurado como escravo, controla um servo motor para a abertura controlada de uma
porta, permitindo a liberação da ração. A comunicação entre os dois microcontroladores foi
estabelecida utilizando USART, proporcionando uma troca eficiente de informações. A
balança digital, integrada ao Arduino Mega por meio do módulo conversor HX711, foi
responsável por monitorar o nível de ração na vasilha.

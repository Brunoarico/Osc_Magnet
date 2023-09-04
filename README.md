# Eletroímãs via OSC

Este é um código desenvolvido para controlar uma matriz de eletroímãs usando o protocolo OSC (Open Sound Control). Ele permite que você controle a potência de cada eletroímã individualmente. A seguir, descrevemos em detalhes as principais funções e seu funcionamento.

## Configurações Iniciais

### Bibliotecas Necessárias

Este código faz uso de várias bibliotecas Arduino e ESP32 para funcionar corretamente. Essas bibliotecas incluem:

- `Arduino.h`
- `ArduinoOTA.h`
- `DNSServer.h`
- `ESPmDNS.h`
- `OSCMessage.h`
- `WebServer.h`
- `WiFi.h`
- `WiFiManager.h`
- `WiFiUdp.h`

Certifique-se de instalar essas bibliotecas no seu ambiente de desenvolvimento antes de compilar o código.

### Constantes e Parâmetros

- `MAGNETS`: Define o número de eletroímãs na matriz.
- `NAME`: Define o nome da placa ESP32.
- `PORT`: Define a porta utilizada para comunicação OSC.
- `RESOLUTION`: Define a resolução dos eletroímãs.
- `PWM_FREQ` e `PWM_RES`: Parâmetros para configuração do PWM.

## Funções Principais

### `initializeBridges()`

Esta função inicializa os eletroímãs e suas configurações PWM. Ela é chamada durante a inicialização do código.

### `initWiFi()`

Esta função configura a conexão Wi-Fi da placa ESP32 e se conecta a uma rede Wi-Fi existente. Caso a conexão seja perdida, ela tentará reconectar.

### `processMagnet(OSCMessage& msg)`

Esta função é chamada quando uma mensagem OSC é recebida. Ela interpreta a mensagem para determinar a potência e a posição do eletroímã e ajusta a saída PWM de acordo.

### `setPower()`

Esta função atualiza a potência dos eletroímãs com base no estado atual armazenado na matriz `states[]`.

### `receiveMsg()`

Esta função verifica a presença de mensagens OSC recebidas via UDP e as direciona para a função `processMagnet()` para processamento.

## Inicialização e Loop Principal

### `setup()`

Esta função é chamada uma vez durante a inicialização do código. Ela inicia a comunicação serial, inicializa os eletroímãs e configura a conexão Wi-Fi.

### `loop()`

O loop principal do código. Ele lida com o recebimento de mensagens OSC e atualiza o estado dos eletroímãs conforme necessário.

## Configuração Automática do Wi-Fi

A função `initAutoWifi()` permite que a placa ESP32 crie um ponto de acesso Wi-Fi caso não consiga se conectar a uma rede existente. O nome da rede Wi-Fi será "eletroima" com a senha "imaimaima". Você pode se conectar a esta rede temporária para configurar o ESP32.

## Observações

- Certifique-se de configurar as constantes `ssid` e `pass` com as credenciais da sua rede Wi-Fi.
- Certifique-se de configurar a matriz `rele[]` com os pinos correspondentes aos seus eletroímãs.

Isso deve fornecer uma visão geral completa do funcionamento do código. Certifique-se de ajustar as configurações e constantes de acordo com suas necessidades antes de carregar o código na placa ESP32.

### Sobre a conexão OSC

#### Função processMagnet(OSCMessage& msg)

A função `processMagnet`` é chamada para processar a mensagem OSC recebida. Ela interpreta a mensagem para determinar a potência e a posição do eletroímã e ajusta a saída PWM de acordo com os valores recebidos. Veja como ela funciona:

```cpp
void processMagnet(OSCMessage& msg) {
    int power = 0;
    int index = 0;

    if (msg.isInt(0))
        power = msg.getInt(0);
    else if (msg.isFloat(0))
        power = msg.getFloat(0);
    else if (msg.isString(0)) {
        char pwr[8];
        msg.getString(1, pwr);
        char pos[8];
        msg.getString(0, pos);
        sscanf(pwr, "%d", &power);
        sscanf(pos, "%d", &index);
    }

    if (index >= 0 && index < ARRAY_SIZE(rele)) {
        if (power >= 0 && power <= 255) {
            ledcWrite(index, power);
            Serial.println("Configured Magnet " + String(rele[index]) + ": Power: " + power);
        } else
            Serial.println("Power out of range");
    } else
        Serial.println("Magnet position out of range");
}
```

A função de processamento da mensagem OSC é responsável por interpretar e aplicar os comandos contidos na mensagem OSC recebida. Abaixo, descrevemos o funcionamento dessa função:

- **Variáveis de Potência e Índice**: Variáveis são inicializadas para armazenar os valores de potência e índice do dispositivo a ser controlado.

- **Interpretação da Mensagem**: O código verifica o tipo de dado na mensagem OSC para determinar se a potência é um número inteiro ou um número de ponto flutuante. Se a mensagem contiver strings, esses valores são extraídos e convertidos adequadamente.

- **Atualização do Dispositivo**: Se o índice estiver dentro do intervalo válido e a potência estiver dentro do intervalo aceitável, a função atualiza o dispositivo correspondente com a potência especificada.

- **Impressão de Informações**: Para fins de depuração, informações são impressas para a porta serial, incluindo detalhes sobre a configuração do dispositivo e a potência definida.

Certifique-se de que as mensagens OSC enviadas correspondam ao formato esperado pelo código e sigam a sintaxe correta para garantir um processamento adequado das mensagens pelo dispositivo ESP32.

## Função de Recepção de Mensagens OSC

A função `receiveMsg()` é responsável por receber pacotes OSC via UDP e processá-los. Abaixo, detalhamos como essa função opera:

```cpp
void receiveMsg() {
    OSCMessage msg;
    int size = Udp.parsePacket();

    if (size > 0) {
        while (size--) msg.fill(Udp.read());

        if (!msg.hasError()) {
            msg.dispatch("/ima", processMagnet);
        } else {
            error = msg.getError();
            Serial.print("error: ");
            Serial.println(error);
        }
    }
}
```

A função de recepção de mensagens OSC é responsável por receber e processar pacotes OSC que são enviados ao dispositivo. Abaixo, descrevemos o funcionamento dessa função:

- **`OSCMessage msg`**: Uma instância da classe `OSCMessage` é criada para armazenar a mensagem OSC recebida.

- **`int size = Udp.parsePacket()`**: A função `parsePacket()` verifica a existência de um pacote UDP disponível para leitura e retorna o tamanho do pacote.

- **Verificação de Tamanho**: Se o tamanho do pacote for maior que zero, isso indica que um pacote UDP foi recebido.

- **Leitura do Pacote**: O código entra em um loop para ler o conteúdo do pacote UDP e preenche a mensagem `msg` com os dados recebidos.

- **Verificação de Erro**: É realizada uma verificação para garantir que não houve erros na mensagem OSC usando `msg.hasError()`. Se não houver erros, a mensagem é encaminhada para a função responsável pelo processamento.

- **Tratamento de Erro**: Se ocorrer algum erro na mensagem, o código captura e imprime o erro para fins de depuração.


# Exemplo de Mensagem OSC para Controlar um Eletroímã

Este exemplo apresenta uma mensagem OSC válida que pode ser usada para controlar um eletroímã por meio do código fornecido. A mensagem OSC contém informações sobre o eletroímã específico a ser controlado e a potência desejada.

## Estrutura da Mensagem OSC

A estrutura básica da mensagem OSC é a seguinte:

``` /ima [Índice do Eletroímã] [Potência] ```


- `/ima`: O endereço OSC que identifica a função ou o dispositivo a ser controlado. Neste caso, usamos "/ima" para representar os eletroímãs.

- `[Índice do Eletroímã]`: O índice do eletroímã que você deseja controlar. Este valor deve corresponder ao índice do eletroímã no código, variando de 0 a 7, dependendo da configuração da sua matriz de eletroímãs.

- `[Potência]`: A potência desejada para o eletroímã. Este valor deve estar dentro do intervalo válido definido em seu código, comumente entre 0 e 255.

## Exemplo de Mensagem

Aqui está um exemplo de mensagem OSC válida para controlar o eletroímã número 3 com uma potência de 150:

``` /ima 3 150 ```

Ao enviar esta mensagem OSC para o dispositivo que executa o código fornecido, o eletroímã número 3 será configurado com uma potência de 150. Lembre-se de ajustar os valores de índice e potência de acordo com suas necessidades reais e as configurações definidas no seu código.



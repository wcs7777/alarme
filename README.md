<h1 align="center" border-bottom="none">Alarme</h1>
<h2 align="center" border-bottom="none"><em>Protótipo de alarme com arduino</em></h2>

## Tabela de conteúdos
1. [Sobre o projeto](#sobre-o-projeto)
2. [Tecnologias](#tecnologias)
3. [Funcionalidades](#funcionalidades)
5. [Como rodar](#como-rodar)
5. [Autor](#autor)

***

## Sobre o projeto
Protótipo de alarme feito com arduino, utilizando bluetooth para enviar os comandos.

***

## Propósito
O projeto foi feito em **2019** para a aula de **Laboratório de Hardware** do curso de **Análise e Desenvolvimento de Sistemas**.

***

## Funcionalidades
- Detectar movimento dentro do limite configurado e disparar o alarme
- Desligar o alarme quando disparado
- Ativar e desativar o alarme
- Alterar a distância mínima de detectação
- Alterar a senha

***

## Tecnologias
- [C++][1]
- [Arduino][2]

***

## Como rodar
Para rodar o projeto são necessários uma placa bom bluetooth, um breadboard, um buzzer, um sensor ultrassônico, dois resistores, fios e configurar conforme o esquema abaixo.  
![esquema](/esquema.png)
Após as configurações é necessário compilar e transferir o código em [alarme.cpp](alarme.cpp) para o arduino.

***

## Autor
Willian Carlos  
<wcs7777git@gmail.com>  
<https://www.linkedin.com/in/williancarlosdasilva/>

[1]: https://www.cplusplus.com/
[2]: https://www.arduino.cc/

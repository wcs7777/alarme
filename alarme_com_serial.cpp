/*
 * Autor: Daiane Thais
 * Autor: Leandro Augusto
 * Autor: Luiz Carlos
 * Autor: Willian Carlos
 *
 * Protótipo de alarme com esp32 e bluetooth low energy
 * Há apenas uma característica para escrita e leitura (feedback da ação)
 * A senha default é 9393
 * A distância mínima default é 50cm
 * O alarme está desativado por padrão
 *
 * |-----------------------------------------------------------------------|
 * |Comando          | Ação                                                |
 * |---------------------------------------------------------------------  |
 * |senha            | Informa a senha (desliga o alarme quando disparado) |
 * |senha outrasenha | Muda a senha                                        |
 * |senha nn         | Muda a distância mínima (dois dígitos)              |
 * |senha nnn        | Muda a distância mínima (três dígitos)              |
 * |senha a          | Ativa o alarme                                      |
 * |senha d          | Desativa o alarme                                   |
 * |-----------------------------------------------------------------------|
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <cctype>
#include <cstdlib>

#define SERVICE_UUID		"98dd3202-7005-45e8-9615-c0f79ee19c04"
#define CHARACTERISTIC_UUID "ea784092-1460-11ea-8d71-362b9e155667"

using std::string;

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
	inline void onWrite(BLECharacteristic *characteristic);
};

void iniciePinos();
void inicieBluetooth();
inline string facaAcao(const string entrada);
inline int codigoAcao(const string entrada);
inline string informeSenha(const string informada);
inline string mudeSenha(const string antiga, const string nova);
inline string ativacaoAlarme(const string senhaInformada, char ativacao);
inline string mudeDistanciaMinima(const string senhaInformada, int distancia);
inline bool temInvasor();
inline double distanciaDaOnda();
inline void emitaOnda();
inline void limpeTrigger();
void dispareAlarme();
void loopAlarme(void *);
void desativeAlarme();
inline void toqueAlarme();

const int led = 2;
const int trigger = 4;
const int echo = 5;
const int buzzer = 13;
const int buzzerChannel = 0;
string senha = "9393";
const int tamanhoSenha = senha.length();
bool alarmeAtivado = false;
bool senhaCorreta = false;
double distanciaMinima = 50.0;
TaskHandle_t taskAlarme;

void setup() {
	Serial.begin(9600);
	iniciePinos();
	inicieBluetooth();
}

void loop() {
	if (temInvasor()) {
		dispareAlarme();
		while (!senhaCorreta) {
			digitalWrite(led, !digitalRead(led));
			delay(250);
		}
		desativeAlarme();
	}
}

void iniciePinos() {
	pinMode(led, OUTPUT);
	pinMode(trigger, OUTPUT);
	pinMode(echo, INPUT);
	pinMode(buzzer, OUTPUT);
	ledcSetup(buzzerChannel, 2000, 8);
	ledcAttachPin(buzzer, buzzerChannel);
	ledcWrite(buzzerChannel, 255);
	ledcWriteTone(buzzerChannel, 0);
}

void inicieBluetooth() {
	BLEDevice::init("Esp32 Alarme");
	BLEServer *server = BLEDevice::createServer();
	BLEService *service = server->createService(SERVICE_UUID);
	service->createCharacteristic(
		CHARACTERISTIC_UUID,
		BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
	)->setCallbacks(new CharacteristicCallbacks());
	service->start();
	server->getAdvertising()->start();
}

void CharacteristicCallbacks::onWrite(BLECharacteristic *characteristic) {
	string entrada = characteristic->getValue();
	Serial.print("Entrada: ");
	Serial.println(entrada.c_str());
	characteristic->setValue(facaAcao(entrada));
	characteristic->notify();
}

string facaAcao(const string entrada) {
	const string senhaInformada = entrada.substr(0, tamanhoSenha);
	switch (codigoAcao(entrada)) {
		case 0:
			return informeSenha(senhaInformada);
		case 1:
			return mudeSenha(
				senhaInformada,
				entrada.substr(tamanhoSenha + 1, tamanhoSenha)
			);
		case 2:
			return ativacaoAlarme(
				senhaInformada,
				toupper(entrada[tamanhoSenha + 1])
			);
		case 3:
			return mudeDistanciaMinima(
				senhaInformada,
				atoi(entrada.substr(tamanhoSenha + 1, 3).c_str())
			);
		default:
			return "Entrada invalida";
	}
}

int codigoAcao(const string entrada) {
	int codigo = -1;
	if (entrada.length() >= tamanhoSenha) {
		if (entrada.length() == tamanhoSenha) {
			codigo = 0;
		} else if (entrada.length() == tamanhoSenha * 2 + 1) {
			codigo = 1;
		} else if (entrada.length() == tamanhoSenha + 2) {
			codigo = 2;
		} else {
			codigo = 3;
		}
	}
	return codigo;
}

string informeSenha(const string informada) {
	if (informada == senha) {
		senhaCorreta = true;
		return "Senha correta";
	} else {
		return "Senha incorreta";
	}
}

string mudeSenha(const string antiga, const string nova) {
	if (antiga == senha && nova.length() == tamanhoSenha) {
		senha = nova;
		return "Senha alterada";
	} else {
		return "Senha invalida!";
	}
}

string ativacaoAlarme(const string senhaInformada, char ativacao) {
	string saida = "Alarme inalterado";
	if (senhaInformada == senha) {
		if (ativacao == 'A') {
			alarmeAtivado = true;
			saida = "Alarme ativado";
		} else if (ativacao == 'D') {
			alarmeAtivado = false;
			saida = "Alarme desativado";
		}
	}
	return saida;
}

string mudeDistanciaMinima(const string senhaInformada, int distancia) {
	if (senhaInformada == senha && distancia > 0) {
		distanciaMinima = (double) distancia;
		return "Distancia alterada";
	} else {
		return "Distancia invalida";
	}
}

bool temInvasor() {
	const double distancia = distanciaDaOnda();
	Serial.print(distanciaMinima);
	Serial.print(" - ");
	Serial.print(distancia);
	Serial.print(" Alarme ");
	Serial.println((alarmeAtivado)? "ativado" : "desativado");
	return (alarmeAtivado && distancia < distanciaMinima);
}

double distanciaDaOnda() {
	/* distancia = tempo * velocidade
	 * tempo = duracao / 2
	 * velocidade do som = 343,5 m/s
	 * velocidade do som = (343,5 * 100)/1000000
	 * velocidade do som = 0,03435 cm/us
	 * ritmo do som	  = 1 / 0,03435
	 * ritmo do som	  = 29,1
	 * distancia = duracao / 2 / ritmo do som
	 * distancia = (duracao * 1) / (2 * ritmo do som)
	 * distancia = duracao / 58,2
	 */
	emitaOnda();
	return (double)pulseIn(echo, HIGH) / 58.2;
}

void emitaOnda() {
	limpeTrigger();
	digitalWrite(trigger, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigger, LOW);
}

void limpeTrigger() {
	digitalWrite(trigger, LOW);
	delayMicroseconds(5);
}

void dispareAlarme() {
	senhaCorreta = false;
	xTaskCreatePinnedToCore(
		loopAlarme,
		"loopAlarme",
		10000,
		NULL,
		1,
		&taskAlarme,
		0
	);
}

void loopAlarme(void *) {
	while (true) {
		toqueAlarme();
	}
}

void desativeAlarme() {
	senhaCorreta = false;
	vTaskDelete(taskAlarme);
	ledcWriteTone(buzzerChannel, 0);
	digitalWrite(led, LOW);
}

void toqueAlarme() {
	const note_t notas[] = {
		NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_F, NOTE_A,
		NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_F, NOTE_A,
		NOTE_A, NOTE_A, NOTE_A, NOTE_F, NOTE_C,

		NOTE_A, NOTE_F, NOTE_C, NOTE_A,
		NOTE_E, NOTE_E, NOTE_E, NOTE_F, NOTE_C,
		NOTE_A, NOTE_F, NOTE_C, NOTE_A,

		NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_Gs, NOTE_G,
		NOTE_D, NOTE_D, NOTE_D, NOTE_A, NOTE_A, NOTE_D, NOTE_D, NOTE_Cs,

		NOTE_C, NOTE_B, NOTE_C, NOTE_A, NOTE_F, NOTE_Gs, NOTE_F, NOTE_A,
		NOTE_C, NOTE_A, NOTE_C, NOTE_E,

		NOTE_A, NOTE_A, NOTE_A, NOTE_A, NOTE_Gs, NOTE_G,
		NOTE_D, NOTE_D, NOTE_D, NOTE_A, NOTE_A, NOTE_D, NOTE_D, NOTE_Cs,

		NOTE_C, NOTE_B, NOTE_C, NOTE_A, NOTE_F, NOTE_Gs, NOTE_F, NOTE_A,
		NOTE_A, NOTE_F, NOTE_C, NOTE_A
	};
	const uint8_t oitavas[] = {
		4, 4, 4, 4, 4, 4, 4, 0,
		4, 4, 4, 4, 4, 4, 4, 0,
		4, 4, 4, 4, 5,

		4, 4, 5, 4,
		5, 5, 5, 5, 5,
		4, 4, 5, 4,

		5, 4, 4, 5, 5, 5,
		5, 5, 5, 0, 4, 5, 5, 5,

		5, 4, 5, 0, 4, 4, 4, 4,
		5, 4, 5, 5,

		5, 4, 4, 5, 5, 5,
		5, 5, 5, 0, 4, 5, 5, 5,

		5, 4, 5, 0, 4, 4, 4, 4,
		4, 4, 5, 4
	};
	const short duracoes[] = {
		-4, -4, 16, 16, 16, 16, 8, 8,
		-4, -4, 16, 16, 16, 16, 8, 8,
		4, 4, 4, -8, 16,

		4, -8, 16, 2,
		4, 4, 4, -8, 16,
		4, -8, 16, 2,

		4, -8, 16, 4, -8, 16,
		16, 16, 8, 8, 8, 4, -8, 16,

		16, 16, 16, 8, 8, 4, -8, -16,
		4, -8, 16, 2,

		4, -8, 16, 4, -8, 16,
		16, 16, 8, 8, 8, 4, -8, 16,

		16, 16, 16, 8, 8, 4, -8, -16,
		4, -8, 16, 2
	};
	const int quantiaNotas = sizeof(notas) / sizeof(notas[0]);
	const int notaInteira = 60000 / 130 * 4;
	int duracaoNota = 0;
	for (int i = 0; i < quantiaNotas; i++) {
		if (duracoes[i] > 0) {
			duracaoNota = notaInteira / duracoes[i];
		} else {
			duracaoNota = notaInteira / -duracoes[i];
			duracaoNota *= 1.5;
		}
		if (oitavas[i] > 0) {
			ledcWriteNote(buzzerChannel, notas[i], oitavas[i]);
		} else {
			ledcWriteTone(buzzerChannel, 0); /* rest */
		}
		delay(duracaoNota * 0.9);
		ledcWriteTone(buzzerChannel, 0);
		delay(duracaoNota * 0.1);
	}
}

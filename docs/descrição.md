# Trabalho Prático - Intel SGX

Tentativas ilimitadas permitidas

## Tarefa

![Reprodutor de vídeo para trabalho-pratico-intel-sgx.mp4](./trabalho-pratico-intel-sgx.mp4)

Essa tarefa é composta de 5 desafios, cada um valendo 2 pontos, na pasta home do seu ambiente virtual você irá encontrar dois arquivos:

- **enclave-atividade.signed.so**: Enclave criado pelo professor, que você tera que interagir.
- **enclave-atividade.edl**: Interface com todas as **OCALLS** e **ECALLS** expostas por esse enclave.

**Você também pode baixar o Enclave por aqui:**

- [enclave-atividade.edl](./enclave-atividade.edl)
- [enclave-atividade.signed.so](./enclave-atividade.signed.so)

Você não tem acesso ao código fonte desse enclave (e nem é necessário), mas terá que interagir com ele para completar os desafios.

1. Primeiro logue no ambiente virtual seguindo esse guia: [Guia para utilização do laboratório virtual .pdf](./Guia%20para%20utilização%20do%20laboratório%20virtual%20.pdf)
2. Logue na maquina virtual utilizando o guia, e clone o projeto base <https://github.com/Lohann/intel-sgx-template>
3. Leia com atenção as instruções no [README.md](https://github.com/Lohann/intel-sgx-template?tab=readme-ov-file#project-structure) do projeto base, compile o projeto para garantir que esta tudo funcionando como deveria.
4. Agora que você sabe que o projeto esta funcionando, copie o arquivo **enclave-atividade.signed.so** para a raiz do projeto.
5. Altere a constante **ENCLAVE_FILENAME** no arquivo **app/app.h** para carregar o **enclave-atividade.signed.so**.
6. Substitua o conteúdo do arquivo **enclave/enclave.edl** pelo conteúdo do arquivo **enclave-atividade.edl** fornecido.
7. Execute **`make clean`** para remover os arquivos compilados e **`make SGX_MODE=SIM main`** para compilar **apenas o aplicativo, não é mais necessário compilar o Enclave.**
8. Rode **./main** para verificar se o projeto esta funcionando.
9. Pronto! agora você esta pronto para começar os desafios.

## Desafios

### 1. Chamar o enclave

Altere o projeto para chamar a ecall **ecall_verificar_aluno** do enclave **enclave-atividade.signed.so**, ela recebe como parametro uma string.

### 2. Quebrar senha

A ecall **ecall_verificar_senha**, recebe como parametro um número inteiro que representa uma senha,  se você fornecer a senha correta irá retorna **0**, ou **-1** se você errar.

A senha é um número inteiro aleatório de até 5 digitos, ou seja um numero entre **0** e **99999**, sua tarefa é descobrir a senha.

### 3. Sequencia Secreta

A ecall **ecall_palavra_secreta**, recebe como parametro um array de 20 letras **`char[20]`**, o enclave tem armazenado dentro dele uma **sequencia de 20 letras secretas** composta apenas de **letras maicusculas**. Toda vez que você chama esse enclave passando um array de palavras, ele irá substituir do seu array TODAS as letras que você errou pelo caracter **- (menos)**, porém ele vai deixar intácto as letras nas posições que você acertou, exemplo:

- Você chamou a **ecall_palavra_secreta** com o array de caracteres **`ABCDEZHIKLMNOPQRSTVX`**
- O enclave alterou o array, e agora ele é igual a **`---DE--I---N------V-`**, quer dizer que você acertou 5 letras.
- Você manteve as letras que acertou, e agora mandou **`QAEDESEIIUJNLUMMWDVJ`**
- O enclave alterou o array, e agora ele é igual a **`---DE--I---N----W-V-`**, você encontrou uma letra nova!
- E assim por diante, até você descobrir a frase secreta inteira.

### 4. Polinômio Secreto

A ecall **ecall_polinomio_secreto** recebe como parametro um numero **`x`**, e ti devolve o resultado da equação de segundo grau **(ax\*\*2 + bx + c) % p** onde **`a`**, **`b`** e **`c`** são valores secretos e **p** é o numero primo [2147483647](https://en.wikipedia.org/wiki/2,147,483,647) (maior primo de 32bit) você pode chamar essa ocall com qualquer valor exceto zero, se você passar zero a chamada irá falhar, sua tarefa é descobrir o **a**, **b** e **c**.

### 5. Pedra, Papel e Tesoura

- [enclave-desafio-5.edl](./enclave-desafio-5.edl)
- [enclave-desafio-5.signed.so](./enclave-desafio-5.signed.so)

Substituia o **enclave-atividade.signed.so** e **enclave-atividade.edl** do projeto pelos fornecidos acima, você não precisa criar um projeto novo, o enclave desse desafio tem a mesma interface do anterior, ele apenas adiciona duas funções:

- **ecall_pedra_papel_tesoura**
- **ocall_pedra_papel_tesoura**

No código **app/app.c** crie a implemente a ocall **`ocall_pedra_papel_tesoura`**, essa ocall será chamada pelo **enclave** quando você chamar a ecall  **ecall_pedra_papel_tesoura**, o enclave ira chamar sua ocall 20 vezes e ela sempre deve retornar um inteiro entre 0 e 2, onde **pedra = 0**, **papel = 1** e **tesoura=2**, (obs: se você não implementar essa ocall, a execução irá falhar) ao final da execução o enclave irá retornar um inteiro representando quantas partidas você ganhou, ou um **valor negativo** caso algum problema tenha ocorrido, você deve ganhar todas as 20 partidas para concluir o desafio, a entrega são todas as jogadas do enclave e suas, que sera impresso no terminal quando o desafio for concluído, exemplo:

```raw
------------------------------------------------
[ENCLAVE] DESAFIO 5 CONCLUIDO!!
          ENCLAVE JOGADAS: 11212201212200121120
             SUAS JOGADAS: 22020012020011202201
------------------------------------------------
```

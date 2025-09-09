# Sistema de arquivos (parte 1)

### arquivos

- uma pasta nada mais é que um arquivo que guarda mais arquivos
- nomes de arquivos podem ou não ter extensões 

### 2 formas de acessar arquivos

- Sequencial
- Aleatória

### pq tamanho do arquivo e tamanho do arquivo em disco?

- por causa dos setores -> crio um arquivo de texto e só tem 1 caracter
- cada caracter é 1 byte, esse arquivo tem 1 byte 
- porém a unidade mínima é um setor, ou seja, 256 bytes
- então o menor tamanho em disco é sempre definido pelos seus setores

## Diretório

### sistema de arquivos de diretório único

- um diretório contendo todos os arquivos

## Implementação de sistema de arquivos

- sistema MBR (Master Boot Record - registro mestre de inicialização)
- discos são divididos em partições
- dual boot, organização etc
- i-nodes: região do sistema de arquivos que guarda as informações dos arquivos e das pastas
- ext4 no linux => quando eu faço um atalho p um arquivo eu faço um atalho pro i-node dele?

### Alocação contínua

- cd dvd bluray mt boa
- problemas de fragmentação
  - quando o disco fica fragmentado e pequenos e não conseguem ter nenhuma faixa contígua grande

### alocação por lista encadeada
- bloco tal aponta pro bloco btal que aponta pro ctal etc
- armazena em qqlr lugar
- tem que passar por todos os blocos p chefar no final (mto lento)

### alocação encadeada usando uma tabela na memória (FAT - File Allocation Table)
- deixa a lista encadeada de blocos
- coloca isso em uma tabela na memória secundária (hashmap? parece um banco de dados com tuplas?)
- lista encadeada armazenada na memória secundária (disco)
- posso armazenar uma cache de 50 megas (por ex) na minha FAT na memória primária (RAM) 

### dúvidas
- quando eu faço um atalho p um arquivo eu faço um atalho p i-node dele?
- FAT parece um banco de dados

começa prox aula a partir de i-nodes

## i-nodes

### Lista os atributos e endereços de disco dos blocos do disco

- O i-node precisa estar na memória só quando o arquivo correspondente estar aberto
- Quando um sistema de arquivos é criado, você já determina a quantidade de i-nodes que eles vão ter

# Parte 2

### Diretórios

- diretórios tbm são i-nodes
- guarda a lista de i-nodes dentro dele

- o b.o do i-node é que vc tem que fazer i/o no disco pra pegar o nome do arquivo dele
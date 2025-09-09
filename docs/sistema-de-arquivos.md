# Sistema de arquivos (parte 1)

https://drive.google.com/file/u/2/d/133sxOIERie3VIBlNunXnkeFPCW6qPPkx/view?usp=drive_link

https://drive.google.com/file/d/1ioUDzNGKiniLZru7ZNflexPWMABc_AxG/view

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

### Duas soluções
- o b.o do i-node é que vc tem que fazer i/o no disco pra pegar o nome do arquivo dele
- guardar tudo relacionado ao arquivo dentro do i-node é mais limpo e ok porém precisa de mais recursos

## Questões sobre o projeto

### journaling

- regiao com nome especial e antes de escrever nos arquivos escrevo aqui o que vou fazer no meu sistema de arquivos
- faço a alteração e escrevo no journal "terminei de fazer a alteração"

### sistema de arquivos virtual

- que nem a memória virtual
- interface genérica para sistemas de arquivos

### tamanho do bloco
- posso ver meus setores de forma agrupada ao invés de separado, vou agrupar meus setores em blocos de 4 por ex
- quanto maior tamanho do bloco (agrupa vários setores) reduz a fragmentação
- porém vc perde mais espaço pra arquivos pequenos
- se vc tiver um arquivo de 10 bytes em setores configurados pra apenas 256 bytes vc perde 246 bytes pra literalmente nada

#include "disco.hpp"

enum COMMANDS{
    CREATE,
    DELETE,
    LIST,
    SORT,
    READ,
    CONCAT,
    EXIT_A
};

struct comando
{
    COMMANDS command;
    std::string nome;
    std::string nome2;
    int tamanho;
    int inicio;
    int fim;
};

comando processar_comando(){
    comando com;
    std::string campo_1;
    std::string campo_2 = "";
    std::string campo_3 = "";
    std::string campo_4 = "";
    /*le a linha formatada*/
    std::string linha;
    std::getline(std::cin, linha);
    //passa os valores da linha para as variaveis
    std::istringstream iss(linha);
    iss >> campo_1 >> campo_2 >> campo_3 >> campo_4;

    if(campo_1.compare("criar") == 0){
        com.command = CREATE;
        com.nome = campo_2;
        com.tamanho = std::stoi(campo_3);
        return com;

    }else if(campo_1.compare("sair") == 0){
        com.command = EXIT_A;
        com.nome = campo_2;
        return com;

    }else if(campo_1.compare("apagar") == 0){
        com.command = DELETE;
        com.nome = campo_2;
        return com;

    }else if(campo_1.compare("ordenar") == 0){
        com.command = SORT;
        com.nome = campo_2;
        return com;

    }else if(campo_1.compare("ler") == 0){
        com.command = READ;
        com.nome = campo_2;
        com.inicio = std::stoi(campo_3);
        com.fim = std::stoi(campo_4);
        return com;

    }else if(campo_1.compare("concatenar") == 0){
        com.command = CONCAT;
        com.nome = campo_2;
        com.nome2 = campo_3;
        return com;

    }else{
        com.command = LIST;
        com.fim = 0;
        com.inicio = 0;
        com.tamanho = 0;
        com.nome = "";
        com.nome2 = "";
        return com;
    }

    com.command = LIST;
    com.fim = 0;
    com.inicio = 0;
    com.tamanho = 0;
    com.nome = "";
    com.nome2 = "";
    return com;
}



int main() {

    bool loop = true;
    Disco* hd = new Disco();
    hd->Add_File_index(std::string("teste"), (unsigned int)1578);
    //hd->Write_File_index(std::string("teste"), 2, (unsigned int)1578);
    //hd->Printar_Bloco(0);
    std::cout << "digite o comando:\n";
    comando comand = processar_comando();
    

    if(comand.command == LIST){
        hd->Listar_Arquivos();
        hd->Print_space();

    }else if(comand.command == CREATE){
        hd->Cria_Arquivo(comand.nome, comand.tamanho);

    }else if(comand.command == DELETE){
        hd->Apaga_Arquivo(comand.nome);

    }else if(comand.command == READ){
        hd->Ler_Arquivo(comand.nome, comand.inicio, comand.fim);

    }else if(comand.command == EXIT_A){
        loop = false;
        Inode inode = hd->Read_inode(comand.nome);
        std::cout << inode.nome << ":\n";
        vector<int> blocos;
        for(int i = 0; i < 64; i++){
            int valor = *(reinterpret_cast<int*>(inode.blocos + (i*4)));
            if(valor >= QNT_BLOCOS){
                break;
            }else{
                blocos.push_back(valor);
            }
        }
        for (int i = 0; i < blocos.size(); i++)
        {
            hd->Printar_Bloco(blocos[i]);
        }
        
    
        //std::cout << inode.nome << " size: " << tamanho << "\n";
    }
    
    //std::cout << "\n";
    hd->Printar_Bloco(0);
    std::cout << "\n";
    hd->Montar();
    //if(hd.Is_inode_free(0)){
    //    char nome[8] = "teste";
    //    hd.Cria_Arquivo(nome, 10);
    //}else{
        //hd.Printar_Inode(0);
    //}
    //hd.Cria_Arquivo(name, 50);
    //hd.Apaga_Arquivo(name);
    
    /*char nome[8] = "teste";
    char exten[4] = "txt";
    char nxt[4] = {0,0,0,0};
    char blcs[256];
    for(int i = 0; i < 256; i++){
        blcs[i] = 0;
    }*/
    //int j = 10; 
    //int k = ceil((j/64.0));
    //if(hd.Is_inode_free(0)){
        //hd.Write_Inode(0, nome, exten, nxt, blcs);
        //std::cout  << " Inode não existe\n";
    //}else{
        //hd.Printar_Inode(0);
        /**
        Inode node = hd.Read_inode(0);
        std::cout << std::string(node.nome);
        std::cout << ".";
        for(int i = 0; i < 4; i++){
            std::cout << node.extensao[i];
        }
        */
        //std::cout << "\n";
        //hd.Delete_Inode(0);
    //}


    //hd.Book_inode(1);
    //hd.Is_inode_free(1);
    //hd.Release_inode(1);
    //hd.Is_inode_free(1);
    //hd.Is_inode_free(2);
    //hd.Is_inode_free(10);
    //hd.Is_inode_free(77);
    //hd.Is_inode_free(32);
    //hd.Is_inode_free(123456);
    //hd.Desmontar();
    /**
    void *hugepage_addr = mmap(NULL, HUGEPAGE_SIZE, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    if (hugepage_addr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Hugepage alocada com sucesso em: %p\n", hugepage_addr);
    printf("tamanho do Inode: %ld\n", sizeof(Inode));
    // Faça algo com a hugepage aqui (opcional)

    // Desalocando a hugepage
    if (munmap(hugepage_addr, HUGEPAGE_SIZE) == -1) {
        perror("munmap");
        return 1;
    }

    printf("Hugepage desalocada com sucesso.\n");
    */
    return 0;
}


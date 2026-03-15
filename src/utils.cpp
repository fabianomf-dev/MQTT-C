#include <exception>
#include <iostream>
#include <string>
#include <limits>

#include "jwt_auth.hpp"
#include "secret_key.hpp"

void show_menu()
{
    std::cout << "\n===== JWT Tool =====\n";
    std::cout << "1. Gerar Secret\n";
    std::cout << "2. Gerar Token\n";
    std::cout << "3. Verificar Token\n";
    std::cout << "4. Extrair Subject\n";
    std::cout << "0. Sair\n";
    std::cout << "Escolha uma opção: ";
}

void clear_input()
{
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main()
{
    try
    {
        while(true)
        {
            show_menu();

            int option;

            if(!(std::cin >> option))
            {
                std::cin.clear();
                clear_input();
                std::cout << "Entrada inválida!\n";
                continue;
            }

            clear_input();

            if(option == 0)
            {
                std::cout << "Saindo...\n";
                break;
            }

            else if(option == 1)
            {
                std::string secret = SecretKeyGenerator::generate_hex(32);

                std::cout << "\nSecret gerada:\n"
                          << secret << "\n";
            }

            else if(option == 2)
            {
                std::string secret;
                std::string user;

                std::cout << "Digite a Secret: ";
                std::getline(std::cin, secret);

                std::cout << "Digite o usuário: ";
                std::getline(std::cin, user);

                JwtAuth jwt(secret, "vps-broker");

                std::string token = jwt.create_token(user);

                std::cout << "\nToken gerado:\n"
                          << token << "\n";
            }

            else if(option == 3)
            {
                std::string secret;
                std::string token;

                std::cout << "Digite a Secret: ";
                std::getline(std::cin, secret);

                std::cout << "Digite o Token: ";
                std::getline(std::cin, token);

                JwtAuth jwt(secret, "vps-broker");
                bool valid = jwt.verify_token(token);
                std::cout << "Token é " << valid << "\n";
                if(jwt.verify_token(token))
                    std::cout << "Token válido\n";
                else
                    std::cout << "Token inválido\n";
            }

            else if(option == 4)
            {
                std::string token;

                std::cout << "Digite o Token: ";
                std::getline(std::cin, token);

                JwtAuth jwt("", "vps-broker");

                auto subject = jwt.get_subject(token);

                if(subject)
                    std::cout << "Subject: " << *subject << "\n";
                else
                    std::cout << "Erro ao extrair subject\n";
            }

            else
            {
                std::cout << "Opção inválida!\n";
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    return 0;
}
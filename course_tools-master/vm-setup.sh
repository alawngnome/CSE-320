#! /usr/bin/env bash

echo
if hash figlet 2> /dev/null; then
    echo "320 Setup" | figlet -f banner
else
    echo "320 Setup"
fi
echo

echo "Updating..."
# General updates
sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get autoremove -y 2>&1 > /dev/null

# Extras
sudo apt-get install -y figlet terminator htop dialog wget 2>&1 > /dev/null

echo "Installing..."
echo "vm-tools" | figlet -f mini
sudo apt-get install -y open-vm-tools-desktop 2>&1 > /dev/null

echo "Installing..."
echo "Git" | figlet -f mini
sudo apt-get install -y git 2>&1 > /dev/null

echo "Installing..."
echo "Gitk" | figlet -f mini
sudo apt-get install -y gitk 2>&1 > /dev/null

echo "Installing..."
echo Git Submit | figlet -f mini
mkdir -p $HOME/.local
mkdir -p $HOME/.local/bin
cp -p git-submit $HOME/.local/bin
chmod +x $HOME/.local/bin/git-submit

echo "Installing..."
echo "Readline" | figlet -f mini
sudo apt-get install -y libreadline-dev libreadline7-dbg readline-doc 2>&1 > /dev/null

echo "Installing..."
echo "Clang" | figlet -f mini
sudo apt-get install -y clang 2>&1 > /dev/null

echo "Installing..."
echo "GDB" | figlet -f mini
sudo apt-get install -y gdb cgdb 2>&1 > /dev/null

echo "Installing..."
echo "Valgrind" | figlet -f mini
sudo apt-get install -y valgrind 2>&1 > /dev/null

echo "Installing..."
echo "GCC and tools" | figlet -f mini
sudo apt-get install -y gcc make binutils 2>&1 > /dev/null

echo "Installing..."
echo "Criterion" | figlet -f mini
sudo add-apt-repository -y ppa:snaipewastaken/ppa
sudo apt-get update
sudo apt-get install -y criterion-dev

dialog --keep-tite --title "Sublime Text"  --yesno "Do you want to install Sublime with plugins?" 5 50

if [ $? -eq 0 ]; then

    # Add Sublime key
    wget -qO - https://download.sublimetext.com/sublimehq-pub.gpg | sudo apt-key add - 2>&1 > /dev/null
    echo "deb https://download.sublimetext.com/ apt/stable/" | sudo tee /etc/apt/sources.list.d/sublime-text.list 2>&1 > /dev/null

    sudo apt-get update -y

    echo "Installing..."
    echo "Sublime Editor" | figlet -f mini
    sudo apt-get install sublime-text 2>&1 > /dev/null

    mkdir -p "$HOME/.config"
    mkdir -p "$HOME/.config/sublime-text-3"
    mkdir -p "$HOME/.config/sublime-text-3/Installed Packages"
    mkdir -p "$HOME/.config/sublime-text-3/Packages"
    mkdir -p "$HOME/.config/sublime-text-3/Packages/User"

    touch "$HOME/.config/sublime-text-3/Packages/User/Package Control.sublime-settings"
    touch "$HOME/.config/sublime-text-3/Installed Packages/Package Control.sublime-package"
    wget -qO - https://packagecontrol.io/Package%20Control.sublime-package > "$HOME/.config/sublime-text-3/Installed Packages/Package Control.sublime-package"

    echo "{\"bootstrapped\":true,\"installed_packages\":[\"Package Control\",\"TrailingSpaces\",\"SublimeLinter\",\"SublimeLinter-contrib-gcc\"]}" > "$HOME/.config/sublime-text-3/Packages/User/Package Control.sublime-settings"
    echo "{\"trailing_spaces_trim_on_save\": true}" > "$HOME/.config/sublime-text-3/Packages/User/trailing_spaces.sublime-settings"
    echo "{\"ignored_packages\":[\"Vintage\"],\"hot_exit\":false,\"save_on_focus_lost\":true,\"translate_tabs_to_spaces\":true}" > "$HOME/.config/sublime-text-3/Packages/User/Preferences.sublime-settings"
    cp -p SublimeLinter.sublime-settings ~/.config/sublime-text-3/Packages/User
fi
echo "-----------------------------"
echo "!ATTN!" | figlet
echo "-----------------------------"
echo -e "If you \e[31;1mcannot\e[0m execute git submit add the following to your ~/.bashrc or other relevant terminal config"
echo "export PATH=\$PATH:$HOME/.local/bin"

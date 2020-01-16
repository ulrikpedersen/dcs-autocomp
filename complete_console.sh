#!/bin/bash

#FULLSCRIPTNAME="$(pwd -P)/${BASH_ARGV[0]}"
FULLSCRIPTNAME="${BASH_ARGV[0]}"
INSTALL_DIR=$( dirname ${FULLSCRIPTNAME} )

_RHEL_VERSION=$(/usr/bin/lsb_release -sr | cut -d. -f1)

_dcs_compgen=${INSTALL_DIR}/dcs_compgen
_redirect_table=/dls_sw/prod/etc/redirector/redirect_table

_dcs_list_support_modules()
{
    echo $(svn ls "${SVN_ROOT}/diamond/trunk/support" | tr -d / | sort )
}

_dcs_list_module_versions()
{
    echo $(svn ls "${SVN_ROOT}/diamond/release/support/$1" | tr -d / | sort )
}

_dcs_list_domains()
{
    echo $( configure-ioc list | awk '{ print $1 }' | grep -o "^[A-Z]\{2\}[0-9]\{2\}[A-Z]\{1\}-" | sort | uniq )
}

_dcs_list_iocs()
{
    echo $(configure-ioc list | awk '{ print $1 }' | sort )
}

_dcs_list_server_iocs()
{
    #echo $(cat /dls_sw/prod/etc/init/$(hostname -s)/soft-iocs | awk '{ print $1 }' | sort )
    echo $(ioc-list | awk '{print $1}' | tr -d : | sort )
}

_dcs_tab_complete_iocs()
{
    local cur prev arg
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    arg=$(_dcs_list_iocs)
    COMPREPLY=( $(${_dcs_compgen} -W "${arg}" ${cur}) )
    return 0
}

_dcs_tab_complete_modules()
{
    local cur prev arg
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    arg=$(_dcs_list_support_modules)
	# Case sensitive version using standard compgen
    #COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) ) 
	# Case insensitive version using custom cis_compgen
    COMPREPLY=( $(${_dcs_compgen} -W "${arg}" -- ${cur}) )
    return 0
}


# Will tab-complete on up to three subsequent arguments: module [version [version]]
# Command structure:
# dls-logs-since-release.py [options] [<module_name> [<earlier_release> [<later_release>]]]
# Options are also handled:
#   -h, --help            show this help message and exit
#   -a AREA, --area=AREA  set <area>=AREA, e.g. support, ioc, matlab, python,
#                         etc, tools, epics
#   -p, --python          set <area>='python'
#   -i, --ioc             set <area>='ioc'
#   -v, --verbose         Print lots of log information
#   -r, --raw             Print raw text (not in colour)
_dls_logs_since_release()
{
    local cur prev arg opts support_module
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Options first
    opts="-h --help -a --area -p --python -i --ioc -v --verbose -r --raw"
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
        
    case "${prev}" in
        "-a"|"--area")
            areas="support ioc matlab python etc tools epics"
            COMPREPLY=( $( compgen -W "${areas}" -- ${cur}) )
            return 0
            ;;
    esac
    
    #echo " prev= ${prev} "
    #echo " SUP= ${DCS_SUPPORT_MODULE} "
    if [[ ${#COMP_WORDS[*]} -gt 2 ]] ; then
        if [ "$DCS_SUPPORT_MODULE" = "${COMP_WORDS[COMP_CWORD-2]}" ] ; then
            
            versions=$( _dcs_list_module_versions ${COMP_WORDS[COMP_CWORD-2]} )
            COMPREPLY=( $(compgen -W "${versions}" -- ${cur}) )
            unset $DCS_SUPPORT_MODULE
            return 0
        fi
    fi 
    if [ "$DCS_SUPPORT_MODULE" = "${prev}" ] ; then
        versions=$( _dcs_list_module_versions ${prev} )
        COMPREPLY=( $(compgen -W "${versions}" -- ${cur}) )
        #unset $DCS_SUPPORT_MODULE
    else
        support_modules=$( _dcs_list_support_modules )
        COMPREPLY=( $(${_dcs_compgen} -W "${support_modules}" ${cur}) )
        if [[ ${#COMPREPLY[*]} -eq 1 ]] ; then
            export DCS_SUPPORT_MODULE="${COMPREPLY[0]}"
        fi
    fi
    
    return 0    
}

_dcs_tab_complete_dls_release()
{
    local cur prev arg opts support_module
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Options first
    opts="--help--area --python --ioc --branch --force --no-test-build --local-build-only --test_build-only --work_build --epics_version --message --next_version --rhel_version --windows"
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
        
    case "${prev}" in
        "-a"|"--area")
            areas="support ioc matlab python etc tools epics"
            COMPREPLY=( $( compgen -W "${areas}" -- ${cur}) )
            return 0
            ;;
        "-b"|"--branch")
            return 0;
            ;;
        "-e"|"--epics_version")
            epics_releases="R3.14.11 R3.14.12.1 R3.14.12.2 R3.14.12.3"
            COMPREPLY=( $( compgen -W "${epics_releases}" -- ${cur}) )
            return 0
            ;;
        "-m"|"--message")
            return 0
            ;;
        "-r"|"--rhel_version")
            rhel_versions="4 5 5_64 6"
            COMPREPLY=( $( compgen -W "${rhel_versions}" -- ${cur}) )
            return 0
            ;;
        "-w"|"--windows")
            win_versions="32 64"
            COMPREPLY=( $( compgen -W "${win_versions}" -- ${cur}) )
            return 0
            ;;
    esac

    support_modules=$( _dcs_list_support_modules )
    COMPREPLY=( $(${_dcs_compgen} -W "${support_modules}" ${cur}) )
    return 0
}

_dcs_tab_complete_server_iocs()
{
    local cur prev arg
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    iocs=$(_dcs_list_server_iocs)
    COMPREPLY=( $(${_dcs_compgen} -W "${iocs}" ${cur}) )
    return 0
}

_dcs_tab_complete_caget()
{
    local cur prev arg opts
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Options first
    opts="-h -w -c -p -t -a -d -n -# -S -e -f -g -s -0x -0o -0b -F"
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
    
    # Handle option arguments
    case "${prev}" in 
        "-p")
            arg=$( seq 0 99 )
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;
        "-d")
            read -d '' arg << "EOF" 
DBR_STRING     DBR_STS_FLOAT   DBR_TIME_LONG   DBR_CTRL_SHORT
DBR_INT        DBR_STS_ENUM    DBR_TIME_DOUBLE DBR_CTRL_INT
DBR_SHORT      DBR_STS_CHAR    DBR_GR_STRING   DBR_CTRL_FLOAT    
DBR_FLOAT      DBR_STS_LONG    DBR_GR_SHORT    DBR_CTRL_ENUM     
DBR_ENUM       DBR_STS_DOUBLE  DBR_GR_INT      DBR_CTRL_CHAR     
DBR_CHAR       DBR_TIME_STRING DBR_GR_FLOAT    DBR_CTRL_LONG     
DBR_LONG       DBR_TIME_INT    DBR_GR_ENUM     DBR_CTRL_DOUBLE   
DBR_DOUBLE     DBR_TIME_SHORT  DBR_GR_CHAR     DBR_STSACK_STRING 
DBR_STS_STRING DBR_TIME_FLOAT  DBR_GR_LONG     DBR_CLASS_NAME    
DBR_STS_SHORT  DBR_TIME_ENUM   DBR_GR_DOUBLE   
DBR_STS_INT    DBR_TIME_CHAR   DBR_CTRL_STRING 
EOF
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;
        "-#"|"-e"|"-f"|"-g")
            return 0
            ;;
    esac
    
    # Finally search for domains, components and records in DB files
    _dcs_find_records ${cur}
    return 0
    
}

_dcs_tab_complete_camonitor()
{
    local cur prev arg opts
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Options first
    opts="-a -h -w -m -p -t -n -# -S -e -f -g -s -0x -0o -0b -F"
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
    
    # Handle option arguments
    case "${prev}" in 
        "-p")
            arg=$( seq 0 99 )
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;
        "-m")
            if [[ ${#cur} -lt 1 ]] ; then
                echo ""
                echo " 'v' (value)"
                echo " 'a' (alarm)"
                echo " 'l' (log/archive)"
                echo " 'p' (property)"
            fi
            arg="v va vl vp val vap valp a al ap alp l lp p"
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;
        "-t")
            arg="sr si sI cr ci cI nr ni nI"
            if [[ ${#cur} -lt 1 ]] ; then
                echo ""
                echo " 's' = CA server (remote) timestamps"
                echo " 'c' = CA client (local) timestamps (shown in '()'s)"
                echo " 'n' = no timestamps"
                echo " 'r' = relative timestamps (time elapsed since start of program)"
                echo " 'i' = incremental timestamps (time elapsed since last update)"
                echo " 'I' = incremental timestamps (time elapsed since last update, by channel)"
                echo ${arg}
            fi
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;

        "-w"|"-#"|"-e"|"-f"|"-g")
            return 0
            ;;
    esac
    
    # Finally search for domains, components and records in DB files
    _dcs_find_records ${cur}
    return 0

}

_dcs_tab_complete_caput()
{
    local cur prev arg opts
    COMREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Options first
    opts="-a -h -w -c -p -t -l -n -s -a -S"
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi
    
    # Handle option arguments
    case "${prev}" in 
        "-p")
            arg=$( seq 0 99 )
            COMPREPLY=( $(compgen -W "${arg}" -- ${cur}) )
            return 0
            ;;
        "-w")
            return 0
            ;;
    esac
    
    # Finally search for domains, components and records in DB files
    _dcs_find_records ${cur}
    return 0

}

# @param $1 current word to complete (cur)
# @modifies global array $COMPREPLY
#
__ltrim_colon_completions()
{
    if [[ "$1" == *:* && "$COMP_WORDBREAKS" == *:* ]]; then
        # Remove colon-word prefix from COMPREPLY items
        local colon_word=${1%${1##*:}}
        local i=${#COMPREPLY[*]}
        while [[ $((--i)) -ge 0 ]]; do
            COMPREPLY[$i]=${COMPREPLY[$i]#"$colon_word"}
        done
    fi
} # __ltrim_colon_completions()

# @param $1 current word (cur) to search through databases
# @modifies global array $COMPREPLY
_dcs_find_records()
{
    _cur="$1"
    #echo " ${_cur}"
    if [[ ${#_cur} -lt 6 ]] ; then
        # Find domains
        domains=$( _dcs_list_domains )
        # Find matches to the entered IOCs
        possible_matches=( `${_dcs_compgen} -W "${domains}" ${_cur}` )
        #echo " pos: ${possible_matches[*]} "
        if [[ ${#possible_matches[*]} -eq 1 ]] ; then
            _cur=${possible_matches[0]}
        else
            COMPREPLY=( ${possible_matches[*]} )
            return 0
        fi
    fi
   
    # If we have at least a full domain name then search through through IOCs and
    # their database files for possible PV matches
    if [[ ${#_cur} -gt 5 ]] ; then
        arg=$( ${_dcs_compgen} -r${_redirect_table} ${_cur} )
        COMPREPLY=( ${arg} )
        return 0
    fi
    return 0
}

complete -F _dcs_tab_complete_iocs console
complete -F _dcs_tab_complete_dls_release dls-release.py
complete -F _dls_logs_since_release dls-logs-since-release.py
complete -F _dcs_tab_complete_server_iocs ioc-connect
complete -F _dcs_tab_complete_caget caget
complete -F _dcs_tab_complete_caput caput
complete -F _dcs_tab_complete_camonitor camonitor

# Big nasty hack which modifies the bash environment to *not* use
# colon (:) as a word separator. This may impact other commands...
# The recommendation is to use __ltrim_colon_completions after setting COMPREPLY
COMP_WORDBREAKS=${COMP_WORDBREAKS//:}


####  Useful code snippets
#databases=$(echo $(configure-ioc list | grep ${domain} | awk '{print $2}' | sed 's/bin.*/db\/\*IOC\*\.db/g')  | sed 's/ /\n/g' | grep -v '\*' | sort | uniq )
# Use a perl regular expression to extract record names from databases.
#records=$(perl -wnE "say for /^\s*record\s*\(.*,\s*\"?(${curr}[A-Za-z0-9:\-]+)\"?\s*\)/g" ${databases})
#domain=${cur:0:5}

# Get list of domains through redirector table
#grep -o "^[A-Z]\{2\}[0-9]\{2\}[A-Z]\{1\}" /dls_sw/prod/etc/redirector/redirect_table | sort | uniq
# Get list of IOCs through redirector table
#grep -o "^[A-Z]\{2\}[0-9]\{2\}[A-Z]\{1\}-[A-Z]\{2\}-IOC-[0-9]\{2\} " /dls_sw/prod/etc/redirector/redirect_table | sort | uniq


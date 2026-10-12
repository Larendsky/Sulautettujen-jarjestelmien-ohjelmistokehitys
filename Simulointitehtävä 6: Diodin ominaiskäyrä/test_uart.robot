*** Settings ***
Library    String
Library    SerialLibrary

*** Variables ***
${com}              COM7    # Muuta omaan COM-porttiisi
${baud}             115200
${board}            nRF5340
${valid_cmd}        000120X
${valid_res}        80X
${invalid_cmd}      001067X
${error_res}        -1X

*** Test Cases ***
Connect Serial
    Log To Console    Connecting to ${board}
    Add Port    ${com}    baudrate=${baud}    encoding=ascii
    Port Should Be Open    ${com}
    Reset Input Buffer
    Reset Output Buffer

Test Valid Time String
    Write Data    ${valid_cmd}    encoding=ascii 
    Log To Console    Sent valid sequence ${valid_cmd}

    # Terminator X on ASCII-koodina 88 (Hex 58)
    ${read} =    Read Until    terminator=X    encoding=ascii 
    Log To Console    Received ${read}
    
    Should Be Equal As Strings    ${read}    ${valid_res}
    Log To Console    Tested ${read} is same as ${valid_res}

Test Invalid Time String
    Write Data    ${invalid_cmd}    encoding=ascii 
    Log To Console    Sent invalid sequence ${invalid_cmd}

    ${read} =    Read Until    terminator=X    encoding=ascii 
    Log To Console    Received ${read}
    
    Should Be Equal As Strings    ${read}    ${error_res}
    Log To Console    Tested ${read} is same as ${error_res}

Disconnect Serial
    Log To Console    Disconnecting ${board}
    [TearDown]    Delete Port    ${com}

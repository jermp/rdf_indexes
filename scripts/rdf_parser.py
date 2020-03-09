def require(char, what, code):
    if char != what:
        print(code + ": error during parsing")
        exit()

def skip_whitespaces(string, pos):
    while string[pos] == ' ' or string[pos] == '\t':
        pos += 1
    return pos

def parse_nt(string):

    subject_pos = 0
    predicate_pos = 0
    object_pos = 0
    end = 0

    EOS = ''

    n = len(string)
    pos = 0

    while pos < n:
        if string[pos] == '<':
            pos += 1
            subject_pos = pos
            break
        pos += 1

    while pos < n:
        if string[pos] == '>':
            pos += 1
            pos = skip_whitespaces(string, pos)
            require(string[pos], '<', "1")
            pos += 1
            predicate_pos = pos
            break
        pos += 1

    while pos < n:
        if string[pos] == '>':
            pos += 1
            pos = skip_whitespaces(string, pos)

            if string[pos] == '<':
                pos += 1
                object_pos = pos
                EOS = '>'
            elif string[pos] == '"':
                object_pos = pos
                pos += 1
                EOS = '\n' # HDT takes everything left
            else:
                print(string)
                print("2: error during parsing")
                exit()

            break

        pos += 1

    while pos < n:
        if string[pos] == EOS:
            end = pos
            break
        pos += 1

    if string[pos] == '\n':
        end = pos - 2

    return (string[subject_pos:predicate_pos-3],
            string[predicate_pos:object_pos - (2 if end == pos - 2 else 3)],
            string[object_pos:end])

def parse_nq(string):

    subject_begin = 0
    subject_end = 0

    predicate_begin = 0
    predicate_end = 0

    object_begin = 0
    object_end = 0

    EOS = ''

    n = len(string)
    pos = 0

    match_required = False

    if string[pos] == '<':
        match_required = True
        pos += 1
        subject_begin = pos

    while pos < n:
        if match_required and string[pos] == '>':
            subject_end = pos
            pos += 1
            require(string[pos], ' ', "1")
            pos += 1
            match_required = False
            break
        elif (not match_required) and string[pos] == ' ':
            subject_end = pos
            pos += 1
            break
        pos += 1

    predicate_begin = pos

    if string[pos] == '<':
        match_required = True
        pos += 1
        predicate_begin = pos

    while pos < n:
        if match_required and string[pos] == '>':
            predicate_end = pos
            pos += 1
            require(string[pos], ' ', "2")
            pos += 1
            match_required = False
            break
        elif (not match_required) and string[pos] == ' ':
            predicate_end = pos
            pos += 1
            break
        pos += 1

    if string[pos] == '<':
        EOS = '>'
    elif string[pos] == '"':
        EOS = '"'
    else:
        EOS = ' '

    pos += 1
    object_begin = pos

    while pos < n:
        if string[pos] == EOS:
            object_end = pos
            break
        pos += 1

    return (string[subject_begin:subject_end],
            string[predicate_begin:predicate_end],
            string[object_begin:object_end])

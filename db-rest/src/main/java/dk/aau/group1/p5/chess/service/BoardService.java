package dk.aau.group1.p5.chess.service;

import java.util.List;
import java.util.Optional;

import org.springframework.stereotype.Service;

import dk.aau.group1.p5.chess.model.Board;
import dk.aau.group1.p5.chess.model.User;
import dk.aau.group1.p5.chess.repository.BoardRepository;
import dk.aau.group1.p5.chess.repository.UserRepository;

@Service
public class BoardService {
    private final BoardRepository repository;
    private final UserRepository urepository;


    public BoardService(BoardRepository boardRepository, UserRepository r)
    {
        repository = boardRepository;
        urepository = r;
    }

    public List<Board> getAll(){
        return repository.findAll();
    }

    public Optional<Board> get(long id) {
        return repository.findById(id);
    }

    public Board save(Board board) {
        assert(board != null);

        var u = board.getUser();
        
        if (u.getAuthentik_user_id() != null){
            var list = urepository.getByAuthentikUserId(u.getAuthentik_user_id());
            User u2;
            if (list.isEmpty())
                u2 = urepository.save(u);
            else 
                u2 = list.get(0);
            board.setUser(u2);
        }

        //if (u.getId() != null) 
            return repository.save(board);
        
    }

        public User getUserFromAuthID(String AuthentikUserID) {
        var list = urepository.getByAuthentikUserId(AuthentikUserID);
        if (list.isEmpty()) {
            return null;
        } else {
            User u = list.get(0);
            return u;
        }
    }
}

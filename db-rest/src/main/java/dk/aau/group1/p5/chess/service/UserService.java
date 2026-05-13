package dk.aau.group1.p5.chess.service;

import java.util.List;
import java.util.Optional;

import org.springframework.stereotype.Service;

import dk.aau.group1.p5.chess.model.User;
import dk.aau.group1.p5.chess.repository.UserRepository;

@Service
public class UserService {
    private final UserRepository repository;

    public UserService(UserRepository userRepository) {
        repository = userRepository;
    }

    public List<User> getAll() {
        return repository.findAll();
    }

    public Optional<User> get(long id) {
        return repository.findById(id);
    }

    public User save(User user) {
        assert (user != null);
        return repository.save(user);
    }

    public User getUserFromAuthID(String AuthentikUserID) {
        var list = repository.getByAuthentikUserId(AuthentikUserID);
        if (list.isEmpty()) {
            return null;
        } else {
            User u = list.get(0);
            return u;
        }
    }

}
